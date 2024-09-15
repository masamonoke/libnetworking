#include <minunit.h>

#include "networking/socket.h"

#include <assert.h>
#include <stdlib.h>

int client_fd;
int server_fd;

#ifdef TCP
int type = SOCK_STREAM;
#else
int type = SOCK_DGRAM;
#endif

static void test_setup(void) {
	bool res;
	const char* port = "100";
	const char* address = "127.0.0.1";

	res = networking_socket_create_server(&server_fd, port, type);
	assert(res);
	res = networking_socket_create_client(&client_fd, address, port, type);
	assert(res);
}

static void test_teardown(void) {
	char buf[1];

	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);
	close(client_fd);

	assert(read(server_fd, buf, sizeof(buf)) < 0);
	assert(read(client_fd, buf, sizeof(buf)) < 0);
}

MU_TEST(test_server_creation) {
	mu_check(server_fd > 0);
}

MU_TEST(test_client_creation) {
	mu_check(client_fd > 0);
}

MU_TEST(test_read_write) { // NOLINT(readability-function-cognitive-complexity)
	pid_t p;
	char* msg = "hello";

	p = fork();
	if (p == 0) {
		send(client_fd, msg, strlen(msg), 0);
		_exit(0);
	} else {
		char buf[8];
		ssize_t readn;
		struct sockaddr_storage their_addr;
		socklen_t their_addr_len;
#ifdef TCP
		int conn_fd;
		conn_fd = accept(server_fd, (struct sockaddr*) &their_addr, &their_addr_len);
		mu_check(conn_fd > 0);
		readn = recv(conn_fd, buf, sizeof(buf), 0);
#else
		readn = recvfrom(server_fd, buf, sizeof(buf), 0, (struct sockaddr *)&their_addr, &their_addr_len);
#endif
		mu_check(readn > 0);
		mu_check((size_t) readn == strlen(msg));
		mu_check(0 == memcmp(buf, msg, strlen(msg)));
	}
	wait(NULL);
}

MU_TEST_SUITE(test_suite) { // NOLINT(readability-function-cognitive-complexity)
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

	MU_RUN_TEST(test_server_creation);
	MU_RUN_TEST(test_client_creation);
	MU_RUN_TEST(test_read_write);
}

int main(void) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return MU_EXIT_CODE;
}
