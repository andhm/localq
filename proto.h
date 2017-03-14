#ifndef  __PROTO_H_
#define  __PROTO_H_

#include "common.h"

#define PROTO_VERSION_1 1
#define PROTO_HEAD_SIZE sizeof(struct lq_proto_s)

#define PROTO_CMD_SHIFT         8
// COMMON
#define PROTO_CMD_REQUEST       1
#define PROTO_CMD_RESPONSE      (PROTO_CMD_REQUEST<<PROTO_CMD_SHIFT)
// STAT
#define PROTO_CMD_STAT_REQUEST  2
#define PROTO_CMD_STAT_RESPONSE (PROTO_CMD_STAT_REQUEST<<PROTO_CMD_SHIFT)

#define PROTO_REQ_TYPE_HEAD     1
#define PROTO_REQ_TYPE_CLASS    2
#define PROTO_REQ_TYPE_FUNC     3
#define PROTO_REQ_TYPE_ARG      4

#define PROTO_RESP_STAT_TYPE_THREAD         1
#define PROTO_RESP_STAT_TYPE_QUEUE          2
#define PROTO_RESP_STAT_TYPE_CONNECTION     3

typedef struct lq_proto_s {
    int16_t version;
    int16_t cmd;
    int32_t length;
} lq_proto_t;

typedef struct lq_proto_resp_s {
    signed error_no;
    char error_msg[32];
} lq_proto_resp_t;

typedef struct lq_proto_req_s {
    int16_t type;
    int32_t length;
} lq_proto_req_t;

typedef lq_proto_req_t lq_proto_resp_stat_t;










#endif  //__PROTO_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
