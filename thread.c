/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "thread.h"
#include "queue.h"
#include "proto.h"
#include <pthread.h>

static lq_thread_t g_thread = {0};
static pthread_mutex_t g_thread_mutex;

void lq_thread_init(int num, queue_handler_pt handler) {
    // for begin, no need lock
    LQ_DEBUG("thread_init. num[%d]", num);
    g_thread.n_max = num;
    g_thread.n_current = 0;
    g_thread.server_start_time = lq_time();
    g_thread.queue_handler = handler;
}

lq_thread_t *lq_get_thread_info() {
    lq_lock_thread();
    lq_thread_t *thread = &g_thread;
    lq_unlock_thread();
    return thread;
}

inline void lq_lock_thread() {
    pthread_mutex_lock(&g_thread_mutex);
    LQ_DEBUG("lock thread success.");
}

inline void lq_unlock_thread() {
    pthread_mutex_unlock(&g_thread_mutex);
    LQ_DEBUG("unlock thread success.");
}

void lq_thread_create() {
    lq_queue_t *queue = lq_get_queue();
    if (queue->num == 0) {
        LQ_DEBUG("no need create thread processor, queue empty.");
        return;
    }

    int pre_thread_process = 0;
    if (g_thread.n_current) {
        pre_thread_process = queue->num/g_thread.n_current;
    }
    LQ_DEBUG("ready to pthread_create. n_current[%d], n_max[%d], pre_thread_process[%d], queue_num[%d]", g_thread.n_current, g_thread.n_max, pre_thread_process, queue->num);
    lq_lock_thread();
    while (g_thread.n_current == 0 || (pre_thread_process > 200 && g_thread.n_current < g_thread.n_max)) {
        LQ_NOTICE("pthread_create. n_current[%d], n_max[%d], pre_thread_process[%d]", g_thread.n_current, g_thread.n_max, pre_thread_process);
        pthread_t tid;
        // detached
        pthread_attr_t  attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        int err = pthread_create(&tid, &attr, lq_thread_processor, NULL);
        if (err != 0) {
            LQ_WARNING("pthread_create failed. errno[%d], error[%s]", err, strerror(err));
            lq_unlock_thread();
            return;
        }

        queue = lq_get_queue();
        ++g_thread.n_current;
        ++g_thread.n_created;
        pre_thread_process = queue->num/g_thread.n_current;
    }
    lq_unlock_thread();
}

void *lq_thread_processor(void *arg) {
    int dq_failed = 0;
    double last_active_time = lq_time();
    while (1) {
        lq_queue_data_t *queue_data;
        if ((queue_data = lq_dequeue()) != NULL) {
            dq_failed = 0;
            LQ_NOTICE("queue data: thread[%lu], size[%d], data[%s]", pthread_self(), queue_data->size, (char *)queue_data->data);
            LQ_DEBUG("queue_handler start.");
            double process_start = last_active_time = lq_time();
            LQ_RET_T ret = g_thread.queue_handler(queue_data);
            FREE_QUEUE_DATA(queue_data);
            double process_end = lq_time();
            double process_use = process_end - process_start;
            lq_lock_thread();
            ++g_thread.n_process;
            g_thread.use_time_total += process_use;
            if (process_use > g_thread.use_time_longest) {
                g_thread.use_time_longest = process_use;
            } else if (process_use < g_thread.use_time_shortest || !g_thread.use_time_shortest) {
                g_thread.use_time_shortest = process_use;
            }
            if (ret == LQ_ERR) {
                ++g_thread.n_process_failed;
            } 
            lq_unlock_thread();
            LQ_DEBUG("queue_handler end.");

        } else {
            // reach 10 min
            if (lq_time() - last_active_time > 600) {
            // if (dq_failed > 100) {
                lq_lock_thread();
                LQ_NOTICE("pthread_exit. thread[%lu]", pthread_self());
                --g_thread.n_current;
                ++g_thread.n_exit;
                lq_unlock_thread();
                pthread_exit(0);
                // not here actually
                return NULL;
            }
            ++dq_failed;
        }
    }
    return NULL;
}

LQ_RET_T lq_queue_processor(void *arg) {
    LQ_DEBUG("queue processor");
    lq_queue_data_t *queue_data = (lq_queue_data_t *)arg;
    int data_size = queue_data->size;
    int proto_req_size = sizeof(lq_proto_req_t);
    LQ_DEBUG("data_size[%d]", data_size);
    if (data_size < proto_req_size) {
        LQ_WARNING("queue_data invalid.");
        return LQ_ERR;
    }
    
    lq_proto_req_t *head = (lq_proto_req_t *)queue_data->data;
    if (head->type != PROTO_REQ_TYPE_HEAD || head->length < proto_req_size) {
        LQ_WARNING("queue_data invalid. type[%d], length[%d]", head->type, head->length);
        return LQ_ERR;
    }

    char *class = NULL, *method = NULL, *data = NULL;
    int len = proto_req_size;
    while (len < head->length) {
        lq_proto_req_t *req = (lq_proto_req_t *)(queue_data->data + len);
        if (req->type == PROTO_REQ_TYPE_CLASS && req->length) {
            LQ_DEBUG("class length: [%d], offset [%d]", req->length, len);
            class = LQ_MALLOC(req->length+1);
            memcpy(class, (char *)req+proto_req_size, req->length);
            class[req->length] = '\0';
            len += req->length;
        } else if (req->type == PROTO_REQ_TYPE_FUNC && req->length) {
            LQ_DEBUG("method length: [%d], offset [%d]", req->length, len);
            method = LQ_MALLOC(req->length+1);
            memcpy(method, (char *)req+proto_req_size, req->length);
            method[req->length] = '\0';
            len += req->length;
        } else if (req->type == PROTO_REQ_TYPE_ARG && req->length) {
            LQ_DEBUG("arg length: [%d], offset [%d]", req->length, len);
            data = LQ_MALLOC(req->length+1);
            memcpy(data, (char *)req+proto_req_size, req->length);
            data[req->length] = '\0';
            len += req->length;
        }
        len += proto_req_size;
    }

    if (class) {
        LQ_NOTICE("class: %s\n", class);
        LQ_FREE(class);
    }
    if (method) {
        LQ_NOTICE("method: %s\n", method);
        LQ_FREE(method);
    }
    if (data) {
        LQ_NOTICE("data: %s\n", data);
        LQ_FREE(data);
    }

    return LQ_OK;
}




















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
