#ifndef  __THREAD_H_
#define  __THREAD_H_

#include "common.h"

typedef LQ_RET_T (*queue_handler_pt)(void *arg);

typedef struct lq_thread_s {
    int16_t n_max;
    int32_t n_current;
    int32_t n_created;
    int32_t n_exit;

    int32_t n_process;
    int32_t n_process_failed;

    double use_time_total;
    double use_time_longest;
    double use_time_shortest;

    // on need lock under
    double server_start_time;
    queue_handler_pt queue_handler;
} lq_thread_t;

void lq_thread_init(int num, queue_handler_pt handler);
void lq_thread_create();
lq_thread_t *lq_get_thread_info();

void *lq_thread_processor(void *arg);

LQ_RET_T lq_queue_processor(void *arg);

inline void lq_lock_thread();
inline void lq_unlock_thread();












#endif  //__THREAD_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
