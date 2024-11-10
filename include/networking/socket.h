#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <arpa/inet.h>

// TODO: rename to conn
// Creates connection socket of type SOCK_STREAM or SOCK_DGRAM and returns true on success.
// Otherwise *client_fd is -1 and returns false.
__attribute__((warn_unused_result, nonnull(1, 2, 3)))
bool networking_socket_create_client(int* client_fd, const char* address, const char* port, int type);

// TODO: rename to listen
// Creates listen socket and returns true on success.
// Otherwise *server_fd is -1 and returns false.
__attribute__((warn_unused_result, nonnull(1, 2)))
bool networking_socket_create_server(int* server_fd, const char* port, int type);

void networking_socket_get_remote_ip(int fd, char buf[]);
