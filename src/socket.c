#include "networking/socket.h"

#include <memory.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

__attribute__((warn_unused_result))
bool networking_socket_create_client(int* client_fd, const char* address, const char* port, int type) {
	struct addrinfo hints;
	struct addrinfo* server_info;
	struct addrinfo* p;
	int rv;

	*client_fd = -1;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = type;

	rv = getaddrinfo(address, port, &hints, &server_info);
	if (rv != 0) {
		fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(rv));
		return false;
	}

	for (p = server_info; p != NULL; p = p->ai_next) {
		int fd;

		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0) {
			continue;
		}

		if (connect(fd, p->ai_addr, p->ai_addrlen) < 0) {
			close(fd);
			continue;
		}

		*client_fd = fd;
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "failed to connect to %s:%s\n", address, port);
		return false;
	}

	freeaddrinfo(server_info);

	return true;
}

__attribute__((warn_unused_result))
bool networking_socket_create_server(int* server_fd, const char* port, int type) {
	int rv;
	const int yes = 1;
	struct addrinfo hints;
	struct addrinfo* server_info;
	struct addrinfo* p;
	const int backlog = 10;

	*server_fd = -1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = type;
	hints.ai_flags = AI_PASSIVE;

	rv = getaddrinfo(NULL, port, &hints, &server_info);
	if (rv != 0) {
		fprintf(stderr, "getaddrinfo() error: %s\n", gai_strerror(rv));
		return false;
	}

	for (p = server_info; p != NULL; p = p->ai_next) {
		int fd;

		fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (fd < 0) {
			continue;
		}

		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
			fprintf(stderr, "setsockopt() error\n");
			return false;
		}

		if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
			close(fd);
			fprintf(stderr, "failed to bind(), trying again with next address...\n");
			continue;
		}

		*server_fd = fd;
		break;
	}

	freeaddrinfo(server_info);

	if (p == NULL) {
		fprintf(stderr, "failed to create socket\n");
		return false;
	}

	if (type != SOCK_DGRAM && listen(*server_fd, backlog) < 0) {
		fprintf(stderr, "failed to listen()\n");
		close(*server_fd);
		return false;
	}

	return true;
}

void networking_socket_get_remote_ip(int fd, char buf[]) {
struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char ip_str[INET6_ADDRSTRLEN];

    if (getpeername(fd, (struct sockaddr *)&addr, &addr_len) == -1) {
        perror("getpeername failed");
        return;
    }

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *s = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
    }

	memcpy(buf, ip_str, strlen(ip_str) + 1);
}
