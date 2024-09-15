#pragma once

#include <stdbool.h>

struct server;

enum networking_server_ret {
	NETWORKING_SERVER_OK,
	NETWORKING_SERVER_MULTIPLEX_ERROR,
	NETWORKING_SERVER_ACCEPT_ERROR,
	NETWORKING_SERVER_HANDLE_ERROR
};

__attribute__((warn_unused_result))
bool networking_server_new(struct server** server, const char* port, bool (*callback) (int, void*));

void networking_server_del(struct server* server);

enum networking_server_ret networking_server_update(struct server* server, void* data);
