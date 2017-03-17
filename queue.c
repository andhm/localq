/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "queue.h"
#include <pthread.h>

static lq_queue_t g_queue = {0};
static pthread_mutex_t g_queue_mutex;

void lq_queue_init(size_t max) {
    // for begin, no need lock
    LQ_DEBUG("queue_init. max[%lu]", max);
    g_queue.max_size = max;
}

lq_queue_t *lq_get_queue() {
    pthread_mutex_lock(&g_queue_mutex);
    // LQ_NOTICE("get_queue. size[%lu], num[%d]", g_queue.size, g_queue.num);
    lq_queue_t *queue = &g_queue;
    pthread_mutex_unlock(&g_queue_mutex);
    return queue;
}

LQ_RET_T lq_enqueue(void *data, size_t size) {
    pthread_mutex_lock(&g_queue_mutex);
    LQ_DEBUG("en_queue. size[%lu], num[%d]", g_queue.size, g_queue.num);
    if (g_queue.size + size > g_queue.max_size) {
        LQ_WARNING("reach queue memory limit.");
        pthread_mutex_unlock(&g_queue_mutex);
        return LQ_ERR;
    }
    lq_queue_data_t *queue_data = LQ_MALLOC(sizeof(lq_queue_data_t));
    if (queue_data == NULL) {
        LQ_WARNING("LQ_MALLOC() failed.");
        pthread_mutex_unlock(&g_queue_mutex);
        return LQ_ERR;
    }
    queue_data->data = data;
    queue_data->size = size;
    queue_data->next = NULL;
    queue_data->prev = g_queue.tail;
    if (g_queue.head == NULL) {
        g_queue.head = queue_data;
    }
    if (g_queue.tail != NULL) {
        g_queue.tail->next = queue_data;
    }
    g_queue.tail = queue_data;

    g_queue.size += size;
    g_queue.num++;

    pthread_mutex_unlock(&g_queue_mutex);
    return LQ_OK;
}

lq_queue_data_t *lq_dequeue() {
    pthread_mutex_lock(&g_queue_mutex);
    // LQ_DEBUG("de_queue. size[%lu], num[%d]", g_queue.size, g_queue.num);
    if (g_queue.num == 0) {
        // LQ_DEBUG("g_queue empty.");
        pthread_mutex_unlock(&g_queue_mutex);
        return NULL;
    }
    lq_queue_data_t *tail_data = g_queue.tail;
    if (g_queue.tail->prev != NULL) {
        g_queue.tail->prev->next = NULL;
    }
    g_queue.tail = g_queue.tail->prev;

    g_queue.size -= tail_data->size;
    g_queue.num--;

    pthread_mutex_unlock(&g_queue_mutex);
    return tail_data;
}

















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
