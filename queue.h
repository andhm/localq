/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#ifndef  __QUEUE_H_
#define  __QUEUE_H_

#include "common.h"

#define FREE_QUEUE_DATA(queue_data)     \
    do {                                \
        if (queue_data->data) {         \
            LQ_FREE(queue_data->data);  \
        }                               \
        LQ_FREE(queue_data);            \
    } while(0);                         \

typedef struct lq_queue_s {
    size_t size;
    int32_t num;
    size_t max_size;
    struct lq_queue_data_s *head;
    struct lq_queue_data_s *tail;
} lq_queue_t;

typedef struct lq_queue_data_s {
    size_t size;
    void *data;
    struct lq_queue_data_s *prev;
    struct lq_queue_data_s *next;
} lq_queue_data_t;

LQ_RET_T lq_enqueue(void *data, size_t size);
lq_queue_data_t *lq_dequeue();
lq_queue_t *lq_get_queue();
void lq_queue_init(size_t max);












#endif  //__QUEUE_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
