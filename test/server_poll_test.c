#include <minunit.h>
#include <sys/wait.h>

#include "networking/server_poll.h"
#include "networking/socket.h"

struct server* server;
const char* msg = "hello";

static bool callback(int conn_fd, void* data) {
	char buf[8];
	ssize_t readn;
	(void) data;

	if (conn_fd < 0) {
		return false;
	}

	readn = recv(conn_fd, buf, sizeof(buf), 0);
	if (readn <= 0) {
		return false;
	}

	if ((size_t) readn != strlen(msg)) {
		return false;
	}

	if (0 != memcmp(msg, buf, strlen(msg))) {
		return false;
	}
	return true;
}

static void test_setup(void) {
	const char* port = "100";
	networking_server_poll_new(&server, port, callback);
}

static void test_teardown(void) {
	networking_server_poll_del(server);
}

MU_TEST(test_handle) {
	pid_t p;

	p = fork();
	if (p == 0) {
		int client_fd;
		mu_check(networking_socket_create_client(&client_fd, "127.0.0.1", "100", SOCK_STREAM) == true);
		send(client_fd, msg, strlen(msg), 0);
		_exit(0);
	} else {
		mu_check(networking_server_poll_update(server, NULL) == NETWORKING_SERVER_OK);
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
