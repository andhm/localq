/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#ifndef  __CONNECTION_H_
#define  __CONNECTION_H_

#include "common.h"
#include "event.h"

typedef struct lq_connection_s {
    int32_t n_max;
    int32_t n_current;
    int32_t n_connected;
    int32_t n_close;

    struct lq_connection_data_s *head;
    struct lq_connection_data_s *tail;
} lq_connection_t;

typedef struct lq_connection_data_s {
    int fd;
    double last_active;
    unsigned will_close;
    struct lq_event_s *event;
    struct lq_connection_data_s *prev;
    struct lq_connection_data_s *next;
} lq_connection_data_t;

void lq_connection_init(int max);
void lq_connection_create(struct lq_event_s *event);
void lq_connection_active(struct lq_event_s *event);
void lq_connection_close(struct lq_event_s *event);
lq_connection_t *lq_get_connection();










#endif  //__CONNECTION_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
