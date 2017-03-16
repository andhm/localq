/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "signal.h"

static signed server_quit = 0;

int lq_signals_init() {
    LQ_DEBUG("signals_init.");
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = &lq_signals_handler;
    act.sa_flags |= SA_RESTART;

    if (sigaction(SIGQUIT, &act, 0) < 0) {
        LQ_WARNING("sigaction() failed. error[%s]", strerror(errno));
    }
}

static void lq_signals_handler(int signo) {
    LQ_DEBUG("signals_handler. signo[%d]", signo);
    server_quit = 1;
}

signed lq_check_server_quit() {
    return server_quit;
}


















/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
