#ifndef  __NETIO_H_
#define  __NETIO_H_

#include "common.h"

void lq_recv(int fd, void *arg);
void lq_send(int fd, void *arg);
void lq_accept(int fd,  void *arg);













#endif  //__NETIO_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
