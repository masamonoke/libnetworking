#include "networking/server.h"

// pthreads -> poll -> select
#ifdef __has_include
#if __has_include(<pthread.h>)
#define SERVER_PTHREAD
#include "networking/server_pthread.h"
#define new(server, port, callback) (networking_server_pthread_new(server, port, callback))
#define del(server) (networking_server_pthread_del(server))
#define update(server, data) (networking_server_pthread_update(server, data))
#elif __has_include(<sys/poll.h>)
#define SERVER_POLL
#include "networking/server_poll.h"
#define new(server, port, callback) (networking_server_poll_new(server, port, callback))
#define del(server) (networking_server_poll_del(server))
#define update(server, data) (networking_server_poll_update(server, data))
#endif
#else
#define SERVER_SELECT
#include "networking/server_select.h"
#define new(server, port, callback) (networking_server_select_new(server, port, callback))
#define del(server) (networking_server_select_del(server))
#define update(server, data) (networking_server_select_update(server, data))
#endif

bool networking_server_new(struct server** server, const char* port, bool (*callback) (int, void*)) {
	return new(server, port, callback);
}

void networking_server_del(struct server* server) {
	del(server);
}

enum networking_server_ret networking_server_update(struct server* server, void* data) {
	return update(server, data);
}
