/*
  +----------------------------------------------------------------------+
  | Author: Min Huang <andhm@126.com>                                    |
  +----------------------------------------------------------------------+
*/

#include "common.h"
#include "proto.h"
#include "thread.h"
#include "queue.h"
#include "connection.h"
#include <sys/socket.h>
#include <sys/un.h>

static void build_time_str(int total_sec, char *buf);

int main() {
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un address;
	address.sun_family = AF_UNIX;
	strcpy(address.sun_path, "../localq.sock");

	int result = connect(sockfd, (struct sockaddr *)&address, sizeof(address));
	if (result == -1) {
		perror("connect failed: ");
		exit(1);
	}
	
	char *str;
	printf("> ");
	while (scanf("%s", str)) {
		if (0 == strcmp(str, "quit")) {
			break;
		}
		if (0 == strcmp(str, "status")) {
			int head_size = sizeof(lq_proto_t);
			lq_proto_t head = {PROTO_VERSION_1, PROTO_CMD_STAT_REQUEST, 0};
			write(sockfd, &head, head_size);
			LQ_DEBUG("ready to read head");
			int read_num = read(sockfd, &head, head_size);
			LQ_DEBUG("read_num: %d", read_num);
			int data_size = head.length;
			LQ_DEBUG("read data length: %d", data_size);
			
			char *result = malloc(data_size);
			read(sockfd, result, data_size);

			int len = 0;
			printf("\n*************************************\n");
			printf("+               Localq              +\n");
			printf("*************************************\n");
			while (len < data_size) {
				lq_proto_resp_stat_t *stat_head = (lq_proto_resp_stat_t*)((char *)result+len);
				len += sizeof(lq_proto_resp_stat_t);
				if (stat_head->type == PROTO_RESP_STAT_TYPE_THREAD) {
					lq_thread_t *thread = (lq_thread_t*)((char *)result + len);
					char *use_time_total_buf = malloc(64);
					memset(use_time_total_buf, 0, 64);
					build_time_str(thread->use_time_total, use_time_total_buf);
					char *server_start_time_buf = malloc(64);
					memset(server_start_time_buf, 0, 64);
					build_time_str(lq_time()-thread->server_start_time, server_start_time_buf);
					printf("最大工作线程数量\t%d\n", thread->n_max);
					printf("当前工作线程数量\t%d\n", thread->n_current);
					printf("历史创建线程数量\t%d\n", thread->n_created);
					printf("历史退出线程数量\t%d\n", thread->n_exit);
					printf("当前已处理请求量\t%d\n", thread->n_process);
					printf("处理失败请求数量\t%d\n", thread->n_process_failed);
					printf("处理请求累计时间\t%s\n", use_time_total_buf);
					printf("处理请求最长时间\t%f\n", thread->use_time_longest);
					printf("处理请求最短时间\t%f\n", thread->use_time_shortest);
					printf("处理请求平均时间\t%f\n", thread->use_time_total/thread->n_process);
					printf("服务已经运行时长\t%s\n", server_start_time_buf);
					free(use_time_total_buf);
					free(server_start_time_buf);
				} else if (stat_head->type == PROTO_RESP_STAT_TYPE_QUEUE) {
					lq_queue_t *queue = (lq_queue_t*)((char *)result + len);
					printf("当前队列请求数量\t%d\n", queue->num);
					printf("当前队列存储容量\t%dM\n", (unsigned long)queue->size/1024/1024);
					printf("当前队列容量上限\t%dM\n", (unsigned long)queue->max_size/1024/1024);
				} else if (stat_head->type == PROTO_RESP_STAT_TYPE_CONNECTION) {
					lq_connection_t *connection = (lq_connection_t*)((char *)result + len);
					printf("当前连接数量上限\t%d\n", connection->n_max);
					printf("当前建立连接数量\t%d\n", connection->n_current);
					printf("历史建立连接数量\t%d\n", connection->n_connected);
					printf("历史关闭连接数量\t%d\n", connection->n_close);
				} else {
					break;
				}
				len += stat_head->length;
			}
			free(result);
			printf("\n\n");
			printf("> ");
			continue;
		}

		int head_size = sizeof(lq_proto_t);
		int data_size = strlen(str);
		LQ_DEBUG("ready to send: %s, %d", str, data_size);
		int send_size = data_size + head_size;
		LQ_DEBUG("send size: %d", send_size);
		char *send_str = malloc(send_size);
		lq_proto_t head = {PROTO_VERSION_1, PROTO_CMD_REQUEST, data_size};
		memcpy(send_str, &head, head_size);
		memcpy((send_str + head_size), str, data_size);
		write(sockfd, send_str, send_size);
		free(send_str);
		LQ_DEBUG("ready to read head");
		
		
		int read_num = read(sockfd, &head, head_size);
		LQ_DEBUG("read_num: %d", read_num);
		data_size = head.length;
		LQ_DEBUG("read data length: %d", data_size);
		
		lq_proto_resp_t *result = malloc(data_size);
		read(sockfd, result, data_size);

		LQ_NOTICE("read data, error_no[%d], error_msg[%s]", result->error_no, result->error_msg);
		printf("> ");
	}
	
	LQ_NOTICE("will close client");
	close(sockfd);

	return 0;
}

static void build_time_str(int total_sec, char *buf) {
	int sec = total_sec%60;
	int min = total_sec/60%60;
	int hour = total_sec/3600%3600;
	int day = total_sec/86400%86400;
	sprintf(buf, "%d天%d时%d分%d秒", day, hour, min, sec);
}

