#include "networking/server_select.h"
#include "networking/socket.h"

#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

struct server_select {
	fd_set master;
	fd_set read_fds;
	int fd_max;
	bool (*callback) (int client_fd, void* data);
	int listener;
};

bool networking_server_select_new(struct server** server, const char* port, bool (*callback) (int, void*)) {
	int listener;
	struct server_select* s_select;

	*server = malloc(sizeof(struct server_select));
	s_select = (struct server_select*) *server;

	FD_ZERO(&s_select->master);
	FD_ZERO(&s_select->read_fds);

	if (!networking_socket_create_server(&listener, port, SOCK_STREAM)) {
		return false;
	}

	FD_SET(listener, &s_select->master);
	s_select->fd_max = listener;
	s_select->listener = listener;

	s_select->callback = callback;

	return true;
}

void networking_server_select_del(struct server* server) {
	int fd;
	struct server_select* s_select;

	s_select = (struct server_select*) server;

	for (fd = 0; fd <= s_select->fd_max; fd++) {
		if (FD_ISSET(fd, &s_select->read_fds)) {
			shutdown(fd, SHUT_RDWR);
			close(fd);
		}
	}

	free(server);
}

enum networking_server_ret networking_server_select_update(struct server* server, void* data) {
	struct server_select* s_select;
	int fd;
	struct sockaddr_storage remote_addr;
	socklen_t addrlen;
	int conn_fd;

	s_select = (struct server_select*) server;

	s_select->read_fds = s_select->master;

	if (select(s_select->fd_max + 1, &s_select->read_fds, NULL, NULL, NULL) < 0) {
		perror("select");
		return NETWORKING_SERVER_MULTIPLEX_ERROR;
	}

	for (fd = 0; fd <= s_select->fd_max; fd++) {
		if (FD_ISSET(fd, &s_select->read_fds)) {
			if (fd == s_select->listener) {
				addrlen = sizeof(remote_addr);
				conn_fd = accept(s_select->listener, (struct sockaddr*) &remote_addr, &addrlen);
				if (conn_fd < 0) {
					fprintf(stderr, "Failed to accept connection");
					return NETWORKING_SERVER_ACCEPT_ERROR;
				} else {
					FD_SET(conn_fd, &s_select->master);
					if (conn_fd > s_select->fd_max) {
						s_select->fd_max = conn_fd;
					}
				}
			} else {
				if (!s_select->callback(fd, data)) {
					shutdown(fd, SHUT_RDWR);
					close(fd);
					FD_CLR(fd, &s_select->master);
					return NETWORKING_SERVER_HANDLE_ERROR;
				}
			}
		}
	}

	return NETWORKING_SERVER_OK;
}
