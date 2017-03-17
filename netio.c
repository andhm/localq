/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "netio.h"
#include "event.h"
#include "proto.h"
#include "queue.h"
#include "thread.h"
#include "connection.h"

static inline void lq_recv_data_process(lq_event_t *event);
static inline char *lq_send_data_process(lq_event_t *event, size_t *send_size);

void lq_accept(int fd, void *arg) {
    struct sockaddr_un sun;
    socklen_t len = sizeof(struct sockaddr_un); 
    int client_fd;
    if ((client_fd = accept(fd, (struct sockaddr*)&sun, &len)) == -1) {
      if (errno == EAGAIN) {
        LQ_NOTICE("accept() not ready");
      } else {
        LQ_WARNING("accept() error. errno[%d], error[%s]", errno, strerror(errno));
      }
      return;
    }

    LQ_DEBUG("accept client. fd[%d]", client_fd);
    lq_set_nonblocking(client_fd);
    
    lq_event_t *event = LQ_MALLOC(sizeof(lq_event_t));
    lq_event_set(event, client_fd, lq_recv, NULL, event, 0);
    lq_event_add(event, EPOLLIN);
}


void lq_recv(int fd, void *arg) {
   lq_event_t *event = (lq_event_t*)arg;
   int len;
   size_t need_read_size = PROTO_HEAD_SIZE, total_read_size = PROTO_HEAD_SIZE, offset = 0;
   lq_proto_t header = {0};
   unsigned read_head = 0;
   while (1) {
        memset(&header, 0, sizeof(lq_proto_t));
        if (!read_head) {
            len = recv(fd, (char*)&header+offset, need_read_size, 0);
        } else {
            len = recv(fd, (char*)event->data+offset, need_read_size, 0);
        }

        if (len < 0) {
            if (errno == EAGAIN) {
                LQ_NOTICE("recv() again");
                continue;
            } else {
                LQ_WARNING("recv() error. errno[%d], error[%s]", errno, strerror(errno));
            }
    
            lq_event_del(event);
            return;

        } else if (len == 0) {
            LQ_DEBUG("close by client. fd[%d]", fd);
            lq_event_del(event);
            return;

        } else {
            LQ_DEBUG("read ok. fd[%d], readsize[%d]", fd, len);
            if (need_read_size != len && need_read_size != 0) {
                LQ_NOTICE("need read again. fd[%d], len[%d], need_read_size[%lu], total_read_size[%lu]", fd, len, need_read_size, total_read_size);
                need_read_size -= len;
                offset += len;
                continue;
            }
            if (!read_head) {
                LQ_DEBUG("read head ok. fd[%d], cmd[%d], version[%d], length[%d]", fd, header.cmd, header.version, header.length);
                if (header.version == PROTO_VERSION_1) {
                    event->head = header;
                    if (header.length > 0) {
                        event->data = LQ_MALLOC(header.length);
                    } else {
                        // length == 0
                        event->len = 0;
                        lq_recv_data_process(event);
                        lq_event_add(event, EPOLLOUT);
                        return;
                    }
                } else {
                    LQ_NOTICE("read head version invalid. cmd[%d], version[%d], length[%d], len[%d]", header.cmd, header.version, header.length, len);
                    lq_event_del(event);
                    return;
                }
                
                need_read_size = total_read_size = header.length;
                read_head = 1;

            } else {
                LQ_DEBUG("read body ok. fd[%d], total_read_size[%lu]", fd, total_read_size);
                event->len = total_read_size;
                
                lq_recv_data_process(event);

                lq_event_add(event, EPOLLOUT);
                return;
            }
        }
   }
}
    
void lq_send(int fd, void *arg) {
    lq_event_t *event = (lq_event_t *)arg;
    size_t send_size = 0;
    char *send_buf = lq_send_data_process(event, &send_size);

    int len;
    size_t need_write_size = send_size, offset = 0;

    while (1) {
        len = send(fd, send_buf, need_write_size, 0);
        if (len < 0) {
            if (errno == EAGAIN) {
                LQ_NOTICE("send() again");
                continue;
            } else {
                LQ_WARNING("send() error. errno[%d], error[%s]", errno, strerror(errno));
            }

            lq_event_del(event);
            LQ_FREE(send_buf);
            return;

        } else if (len == 0) {
            LQ_DEBUG("close by client. fd[%d]", fd);
            lq_event_del(event);
            LQ_FREE(send_buf);
            return;

        } else {
            if (need_write_size != len && need_write_size != 0) {
                LQ_NOTICE("need write again. fd[%d], len[%d], need_write_size[%lu]", fd, len, need_write_size);
                need_write_size -= len;
                offset += len;
                continue;
            }
            LQ_DEBUG("send ok. fd[%d], len[%d], send_size[%d], body_size[%d]", fd, len, send_size, event->len);
            if (event->connection->will_close) {
                LQ_NOTICE("too many connections. close now, fd[%d]", fd);
                lq_event_del(event);
                LQ_FREE(send_buf);
                return;
            }
            if (event->data) {
                LQ_FREE(event->data);
                event->data = NULL;
            }
            // lq_event_set(event, fd, lq_recv, event->data, event, 0);
            event->handler = lq_recv;
            event->len = 0;
            lq_event_add(event, EPOLLIN);
            LQ_FREE(send_buf);
            return;
        }
    }
}

static inline void lq_recv_data_process(lq_event_t *event) {
    lq_proto_t head = event->head;
    LQ_DEBUG("head info, version[%d], cmd[%d], length[%d]", head.version, head.cmd, head.length);
    if (head.version != PROTO_VERSION_1 || 
        (head.cmd != PROTO_CMD_REQUEST && head.cmd != PROTO_CMD_STAT_REQUEST)) {
        lq_proto_resp_t *proto_resp = LQ_MALLOC(sizeof(lq_proto_resp_t));
        memset(proto_resp, 0, sizeof(lq_proto_resp_t));
        char *error_msg = "not support";
        strncpy(proto_resp->error_msg, error_msg, strlen(error_msg));
        proto_resp->error_no = LQ_ERR;

        event->len = sizeof(*proto_resp);
        event->data = proto_resp;
        event->handler = lq_send;
        return;
    }
    if (head.cmd == PROTO_CMD_REQUEST) {
        lq_proto_resp_t *proto_resp = LQ_MALLOC(sizeof(lq_proto_resp_t));
        memset(proto_resp, 0, sizeof(lq_proto_resp_t));
        char *error_msg = "ok";
        LQ_RET_T ret = LQ_OK;
        
        if (event->connection->will_close) {
            ret = LQ_ERR;
            error_msg = "too many connections";
        } else {
            // en queue
            ret = lq_enqueue(event->data, event->len);
            if (ret == LQ_ERR) {
                error_msg = "reach max memory limit";
            }
            // thread
            lq_thread_create();
        }
        
        strncpy(proto_resp->error_msg, error_msg, strlen(error_msg));
        proto_resp->error_no = ret;

        event->len = sizeof(*proto_resp);
        event->data = proto_resp;
        event->handler = lq_send;
        return;
    
    } else if (head.cmd == PROTO_CMD_STAT_REQUEST) {
        int total_size = sizeof(lq_proto_resp_stat_t)*3 + sizeof(lq_thread_t) + sizeof(lq_queue_t) + sizeof(lq_connection_t);
        char *stat_header = LQ_MALLOC(total_size);
        // thread
        lq_thread_t *gthread = lq_get_thread_info();
        lq_proto_resp_stat_t proto_resp_stat = {PROTO_RESP_STAT_TYPE_THREAD, sizeof(lq_thread_t)};
        int offset = 0;
        memcpy(stat_header, &proto_resp_stat, sizeof(lq_proto_resp_stat_t));
        offset += sizeof(lq_proto_resp_stat_t);
        memcpy(stat_header+offset, gthread, sizeof(lq_thread_t));
        // queue
        lq_queue_t *gqueue = lq_get_queue();
        proto_resp_stat.type = PROTO_RESP_STAT_TYPE_QUEUE;
        proto_resp_stat.length = sizeof(lq_queue_t);
        offset += sizeof(lq_thread_t);
        memcpy(stat_header+offset, &proto_resp_stat, sizeof(lq_proto_resp_stat_t));
        offset += sizeof(lq_proto_resp_stat_t);
        memcpy(stat_header+offset, gqueue, sizeof(lq_queue_t));
        // connection
        lq_connection_t *gconnection = lq_get_connection();
        proto_resp_stat.type = PROTO_RESP_STAT_TYPE_CONNECTION;
        proto_resp_stat.length = sizeof(lq_connection_t);
        offset += sizeof(lq_queue_t);
        memcpy(stat_header+offset, &proto_resp_stat, sizeof(lq_proto_resp_stat_t));
        offset += sizeof(lq_proto_resp_stat_t);
        memcpy(stat_header+offset, gconnection, sizeof(lq_connection_t));

        event->len = total_size;
        event->data = stat_header;
        event->handler = lq_send;
        return;
    }
}

static inline char *lq_send_data_process(lq_event_t *event, size_t *send_size) {
    *send_size = PROTO_HEAD_SIZE + event->len;
    char *send_buf = LQ_MALLOC(*send_size);
    lq_proto_t head;
    head.version = PROTO_VERSION_1;
    head.length = event->len;
    head.cmd = event->head.cmd << PROTO_CMD_SHIFT;
    memcpy(send_buf, &head, PROTO_HEAD_SIZE);
    memcpy(send_buf+PROTO_HEAD_SIZE, event->data, event->len);
    return send_buf;
}












/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
