/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "event.h"
#include "connection.h"
#include "netio.h"
#include "signal.h"

static int g_ep_fd = -1;

void lq_event_init(int size) {
    g_ep_fd = epoll_create(size);
    if (g_ep_fd <= 0) {
        LQ_WARNING("create epoll failed. g_ep_fd: %d", g_ep_fd);
    }
}

void lq_event_add(lq_event_t *event, int events) {
   struct epoll_event ep_ev = {0, {0}};
   int op;
   ep_ev.data.ptr = event;
   ep_ev.events = event->events = events;
   if (event->status == 1) {
        op = EPOLL_CTL_MOD;
        if (events & EPOLLIN) {
            lq_connection_active(event);
        }
   } else {
       op = EPOLL_CTL_ADD;
       event->status = 1;  
       lq_connection_create(event);      
   }

   if (epoll_ctl(g_ep_fd, op, event->fd, &ep_ev) < 0) {
       LQ_WARNING("event add failed[fd=%d], events[%d], errno[%d], error[%s]", event->fd, events, errno, strerror(errno));
   } else {
       LQ_DEBUG("event add ok[fd=%d], op[%d], events[%d]", event->fd, op, events);
   }
}

void lq_event_del(lq_event_t *event) {
    LQ_DEBUG("event del, fd[%d]", event->fd);
    struct epoll_event ep_ev = {0, {0}};
    if (event->status != 1) {
        LQ_DEBUG("event del failed. event not in epoll.");
        return;
    }
    /*ep_ev.data.ptr = event;
    event->status = 0;*/
    epoll_ctl(g_ep_fd, EPOLL_CTL_DEL, event->fd, &ep_ev);
    LQ_DEBUG("close fd[%d]", event->fd);
    close(event->fd);
    lq_connection_close(event);
    if (event->data) {
        LQ_DEBUG("free event data, %p", event->data);
        LQ_FREE(event->data);
    }
    LQ_DEBUG("free event.");
    LQ_FREE(event);
}

void lq_event_set(lq_event_t *event, int fd, event_handler_pt handler, void *data, void *arg, unsigned is_listen_sock) {
    LQ_DEBUG("event set, fd[%d], handler[%p], data[%p], arg[%p], is_listen_sock[%d]", fd, handler, data, arg, is_listen_sock);
    event->fd = fd;
    event->handler = handler;
    event->events = 0;
    event->status = 0;
    event->len = 0;
    event->listen = is_listen_sock;
    event->arg = arg;
    event->data = data;
    event->connection = NULL;
}

void lq_event_loop(sock) {
    LQ_DEBUG("add listenfd to epoll. listenfd[%d]", sock);
    lq_set_nonblocking(sock);
    lq_event_t *event = LQ_MALLOC(sizeof(lq_event_t));
    lq_event_set(event, sock, lq_accept, NULL, event, 1);
    lq_event_add(event, EPOLLIN);

    LQ_NOTICE("epoll loop start.");
    struct epoll_event ep_events[20];
    int nfds;
    while (!lq_check_server_quit()) {
        nfds = epoll_wait(g_ep_fd, ep_events, 20, 5000);
        LQ_DEBUG("epoll_wait returns. nfds[%d]", nfds);
        int i;
        for (i = 0; i < nfds; ++i) {
            lq_event_t *event = ep_events[i].data.ptr;
            
            if (ep_events[i].events & (EPOLLIN|EPOLLOUT)) {
                event->handler(event->fd, event->arg);
            
            } else {
                LQ_WARNING("epoll event error. events[%d]", ep_events[i].events);
            }
            
        }

        // close idle connections
        lq_connection_data_t *connection_data = NULL;
        lq_connection_t *connection = lq_get_connection();
        double now = lq_time();
        int max_try_times = 25;
        for (connection_data = connection->tail; max_try_times > 0 && connection_data; connection_data = connection_data->prev, --max_try_times) {
            if (now - connection_data->last_active < 300) { // 5 mins
                break;
            }
            LQ_NOTICE("close idle connection. fd[%d]", connection_data->fd);
            if (connection_data->event->listen) {
                LQ_NOTICE("is listen fd, not close.");
                continue;
            }

            LQ_DEBUG("connection info. fd[%d], will_close[%d], last_active[%f], now[%f]", connection_data->fd, connection_data->will_close, connection_data->last_active, lq_time());

            lq_event_del(connection_data->event);
        }
    }

    lq_event_del(event);
}






/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
