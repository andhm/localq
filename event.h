#ifndef  __EVENT_H_
#define  __EVENT_H_

#include "common.h"
#include "proto.h"
#include "connection.h"
#include <sys/epoll.h>

typedef void (*event_handler_pt)(int fd, void *arg); 

typedef struct lq_event_s {
    int fd;
    int status; // 1 in epoll; 0 not in epoll
    int events;
    int len;
    unsigned listen;
    event_handler_pt handler;
    void *arg;
    void *data;
    lq_proto_t head;
    struct lq_connection_data_s *connection;
} lq_event_t;

/*void lq_handler_error(lq_event_t *event) {
    LQ_WARNING("error handler, close sock: %d, free event: %p\n", event->fd, event);
    close(event->fd);
    if (event->data) {
        LQ_FREE(event->data);
    }
    LQ_FREE(event);
}*/

static void lq_set_nonblocking(int sock) {
    int opts = fcntl(sock, F_GETFL);
    if (opts < 0) {
        LQ_WARNING("fcntl(sock, GETFL)");
        return;
    }
    opts = opts|O_NONBLOCK;
    if (fcntl(sock, F_SETFL, opts) < 0) {
        LQ_WARNING("fcntl(sock, SETFL, opts)");
        return;
    }
}

void lq_event_init(int size);
void lq_event_set(lq_event_t *event, int fd, event_handler_pt handler, void *data, void *arg, unsigned is_listen_sock);
void lq_event_add(lq_event_t *event, int events);
void lq_event_del(lq_event_t *event);

void lq_event_loop(int sock);










#endif  //__EVENT_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
