#pragma once

#include "server.h"

bool networking_server_pthread_new(struct server** server, const char* port, bool (*callback) (int, void*));

void networking_server_pthread_del(struct server* server);

enum networking_server_ret networking_server_pthread_update(struct server* server, void* data);
