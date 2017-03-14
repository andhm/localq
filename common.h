/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#ifndef  __COMMON_H_
#define  __COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/time.h>

#define LQ_RET_T	signed
#define LQ_OK		0
#define LQ_ERR		-1

#define LQ_MALLOC(size) malloc(size)
#define LQ_FREE(ptr) free(ptr)

#ifdef DEBUG
#define LQ_DEBUG(fmt, arg...) printf("[DEBUG][%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg)  
#else
#define LQ_DEBUG(fmt, arg...)
#endif

#define LQ_NOTICE(fmt, arg...) printf("[NOTICE][%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg)  
#define LQ_WARNING(fmt, arg...) printf("[WARNING][%s:%d] "fmt"\n", __FILE__, __LINE__, ##arg)  

static inline double lq_time() {
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec + (double)now.tv_usec/1000000;
}














#endif  //__COMMON_H_
