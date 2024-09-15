#include "networking/server_pthread.h"
#include "networking/socket.h"
#include "networking/utils/thread_pool.h"

#include <stdlib.h>
#include <memory.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

struct server_pthread {
	int fd;
	// TODO: probably callback should return nothing
	bool (*callback) (int, void*);
	thread_pool_t* tp;
	// polling needed to accept connections
	// and while waiting be able to get int signal
	struct pollfd pfd;
};

bool networking_server_pthread_new(struct server** server, const char* port, bool (*callback) (int, void*)) {
	struct server_pthread* s_pthread;

	*server = malloc(sizeof(struct server_pthread));
	s_pthread = (struct server_pthread*) *server;

	if (!networking_socket_create_server(&s_pthread->fd, port, SOCK_STREAM)) {
		return false;
	}

	s_pthread->callback = callback;
	s_pthread->tp = thread_pool_new(4);
	s_pthread->pfd.fd = s_pthread->fd;
	s_pthread->pfd.events = POLLIN;

	return true;
}

void networking_server_pthread_del(struct server* server) {
	thread_pool_delete(((struct server_pthread*) server)->tp);
	free(server);
}

static void thread_routine(void* args);

enum networking_server_ret networking_server_pthread_update(struct server* server, void* data) {
	struct sockaddr_storage remote_addr;
	socklen_t addrlen = sizeof(remote_addr);
	struct server_pthread* s_pthread;
	int conn_fd;
	int poll_ret;

	s_pthread = (struct server_pthread*) server;

	poll_ret = poll(&s_pthread->pfd, 1, 0);

	if (poll_ret < 0) {
		return NETWORKING_SERVER_ACCEPT_ERROR;
	}

	if (s_pthread->pfd.revents & POLLIN) {
		conn_fd = accept(s_pthread->fd, (struct sockaddr*) &remote_addr, &addrlen);

		if (conn_fd < 0) {
			char hoststr[NI_MAXHOST];
			char portstr[NI_MAXSERV];
			if (0 == getnameinfo((struct sockaddr *)&remote_addr, addrlen, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV)) {
				fprintf(stderr, "accept error on address: %s:%s, error: %d", hoststr, portstr, errno);
			}

			return NETWORKING_SERVER_ACCEPT_ERROR;
		} else {
			int* conn_fd_ptr;
			void** args;

			args = malloc(sizeof(void*) * 3);
			conn_fd_ptr = malloc(sizeof(int));
			memcpy(conn_fd_ptr, &conn_fd, sizeof(int));
			args[0] = conn_fd_ptr;
			args[1] = data;
			args[2] = (void*) s_pthread->callback;
			thread_pool_add_work(s_pthread->tp, thread_routine, args);
		}
	}

	return NETWORKING_SERVER_OK;
}

// args is void* arr[2] where first element is conn_fd, second element is data, third element is callback ptr
static void thread_routine(void* args) {
	void* args_arr[3];
	int conn_fd;
	bool (*callback)(int, void*);
	void* data;

	memcpy(args_arr, args, sizeof(void*) * 3);
	conn_fd = *((int*) args_arr[0]);
	data = args_arr[1];
	callback = (bool (*)(int, void*)) args_arr[2];

	if (!callback(conn_fd, data)) {
		shutdown(conn_fd, SHUT_RDWR);
		close(conn_fd);
	}

	free(args_arr[0]);
	free(args);
}
