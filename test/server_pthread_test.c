#include <minunit.h>
#include <sys/wait.h>

#include "networking/server_pthread.h"
#include "networking/socket.h"

struct server* server;
const char* msg = "hello";

static bool callback(int conn_fd, void* data) {
	char buf[8];
	ssize_t readn;
	(void) data;
	char resp;

	if (conn_fd < 0) {
		return false;
	}

	resp = 1;

	readn = recv(conn_fd, buf, sizeof(buf), 0);
	if (readn <= 0) {
		resp = 0;
		goto notify;
	}

	if ((size_t) readn != strlen(msg)) {
		resp = 0;
		goto notify;
	}

	if (0 != memcmp(msg, buf, strlen(msg))) {
		resp = 0;
		goto notify;
	}

notify:
	send(conn_fd, &resp, sizeof(resp), 0);
	return true;
}

static void test_setup(void) {
	const char* port = "100";
	networking_server_pthread_new(&server, port, callback);
}

static void test_teardown(void) {
	networking_server_pthread_del(server);
}

MU_TEST(test_handle) { // NOLINT(readability-function-cognitive-complexity)
	pid_t p;

	p = fork();
	if (p == 0) {
		int client_fd;
		char resp;

		mu_check(networking_socket_create_client(&client_fd, "127.0.0.1", "100", SOCK_STREAM) == true);
		send(client_fd, msg, strlen(msg), 0);
		recv(client_fd, &resp, sizeof(resp), 0);
		mu_check(resp == 1);
		_exit(0);
	} else {
		struct timespec ts;
		ts.tv_sec = 1;
		ts.tv_nsec = 0;
		nanosleep(&ts, NULL); // sleep to avoid recv deadlock
		mu_check(networking_server_pthread_update(server, NULL) == NETWORKING_SERVER_OK);
	}
	wait(NULL);
}

MU_TEST_SUITE(test_suite) {
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

	MU_RUN_TEST(test_handle);
}

int main(void) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return MU_EXIT_CODE;
}
