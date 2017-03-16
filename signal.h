#ifndef  __SIGNAL_H_
#define  __SIGNAL_H_

#include "common.h"
#include <signal.h>

int lq_signals_init();
static void lq_signals_handler(int signo);

signed lq_check_server_quit();







#endif  //__SIGNAL_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
