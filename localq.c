#include "common.h"
#include "proto.h"
#include "thread.h"
#include "event.h"
#include "queue.h"
#include <sys/socket.h>
#include <sys/un.h>

int main() {
	int server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);

	struct sockaddr_un server_addr;
	server_addr.sun_family = AF_UNIX;
	strcpy(server_addr.sun_path, "localq.sock");
	if (-1 == bind(server_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))) {
		LQ_WARNING("bind() failed. %s", strerror(errno));
		return 1;
	}
	if (-1 == listen(server_sockfd, 5)) {
		LQ_WARNING("listen() failed. %s", strerror(errno));
		return 1;
	}

	LQ_NOTICE("listening fd[%d]", server_sockfd);

	lq_connection_init(100);

	lq_thread_init(2, lq_queue_processor);

	lq_queue_init(100*1024*1024); // 100M
	
	lq_event_init(128);
	
	lq_event_loop(server_sockfd);

	return 0;
}
