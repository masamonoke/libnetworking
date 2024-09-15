#pragma once

#include "server.h"

bool networking_server_select_new(struct server** server, const char* port, bool (*callback) (int, void*));

void networking_server_select_del(struct server* server);

enum networking_server_ret networking_server_select_update(struct server* server, void* data);
