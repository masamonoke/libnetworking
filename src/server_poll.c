#include "networking/server_poll.h"
#include "networking/socket.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <poll.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>

struct server_poll {
	int fd;
	struct pollfd* pfds;
	uint16_t pfd_count;
	uint16_t pfd_capacity;
	socklen_t addrlen;
	struct sockaddr_storage remote_addr;
	bool (*callback) (int client_fd, void* data);
};

bool networking_server_poll_new(struct server** server, const char* port, bool (*callback) (int, void*)) {
	struct server_poll* s_poll;

	*server = malloc(sizeof(struct server_poll));
	s_poll = (struct server_poll*) *server;

	if (s_poll == NULL || !networking_socket_create_server(&s_poll->fd, port, SOCK_STREAM)) {
		return false;
	}

	s_poll->callback = callback;
	s_poll->pfd_count = 0;
	s_poll->pfd_capacity = 5;
	s_poll->pfds = malloc(sizeof(struct pollfd) * (s_poll)->pfd_capacity);
	if (s_poll->pfds == NULL) {
		free(s_poll);
		s_poll = NULL;
		return false;
	}

	s_poll->pfds[0].fd = (s_poll)->fd;
	s_poll->pfds[0].events = POLLIN;
	s_poll->pfd_count++;

	return  true;
}

void networking_server_poll_del(struct server* server) {
	size_t i;
	struct server_poll* s_poll;

	s_poll = (struct server_poll*) server;

	for (i = 0; i < s_poll->pfd_count; i++) {
		shutdown(s_poll->pfds[i].fd, SHUT_RDWR);
		close(s_poll->pfds[i].fd);
	}

	free(s_poll->pfds);
	free(s_poll);
}

__attribute__((warn_unused_result))
static bool accept_connection(struct server_poll* server);

__attribute__((warn_unused_result))
static bool handle_client(struct server_poll* server, void* data, uint16_t pfd_idx);

// TODO: remove true and false returns
enum networking_server_ret networking_server_poll_update(struct server* server, void* data) {
	uint16_t i;
	int poll_count;
	struct server_poll* s_poll;

	s_poll = (struct server_poll*) server;

	poll_count = poll(s_poll->pfds, s_poll->pfd_count, 5000);
	if (poll_count < 0) {
		perror("poll");
		return NETWORKING_SERVER_MULTIPLEX_ERROR;
	}

	for (i = 0; i < s_poll->pfd_count; i++) {
		if (s_poll->pfds[i].fd == s_poll->fd) {
			if (!accept_connection(s_poll)) {
				return NETWORKING_SERVER_ACCEPT_ERROR;
			}
		} else {
			if (!handle_client(s_poll, data, i)) {
				return NETWORKING_SERVER_HANDLE_ERROR;
			}
		}
	}

	return NETWORKING_SERVER_OK;
}

static void add_connection(struct pollfd* pfds[], int conn_fd, uint16_t* len, uint16_t* capacity);

static void del_connection(struct pollfd pfds[], uint16_t idx, uint16_t* len);

static bool accept_connection(struct server_poll* server) {
	int conn_fd;

	server->addrlen = sizeof(server->remote_addr);
	conn_fd = accept(server->fd, (struct sockaddr*) &server->remote_addr, &server->addrlen);
	if (conn_fd < 0) {
		char hoststr[NI_MAXHOST];
		char portstr[NI_MAXSERV];
		if (0 == getnameinfo((struct sockaddr *)&server->remote_addr, server->addrlen, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV)) {
			fprintf(stderr, "accept error on address: %s:%s, error: %d", hoststr, portstr, errno);
		}

		return false;
	} else {
		add_connection(&server->pfds, conn_fd, &server->pfd_count, &server->pfd_capacity);
	}

	return true;
}

static bool handle_client(struct server_poll* server, void* data, uint16_t pfd_idx) {
	int client_fd;
	client_fd = server->pfds[pfd_idx].fd;
	if (!server->callback(client_fd, data)) {
		shutdown(client_fd, SHUT_RDWR);
		close(client_fd);
		del_connection(server->pfds, pfd_idx, &server->pfd_count);
		return false;
	}

	return true;
}

static void add_connection(struct pollfd* pfds[], int conn_fd, uint16_t* len, uint16_t* capacity) {
	if (*len == *capacity) {
		*capacity *= 2;
		*pfds = realloc(*pfds, sizeof(**pfds) * (*capacity));
	}

	(*pfds)[*len].fd = conn_fd; // append
	(*pfds)[*len].events = POLLIN;
	*len = *len + 1;
}

static void del_connection(struct pollfd pfds[], uint16_t idx, uint16_t* len) {
	pfds[idx] = pfds[*len - 1];
	*len = *len - 1;
}
