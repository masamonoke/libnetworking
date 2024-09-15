#include <minunit.h>

#include "networking/socket.h"
#include "networking/io.h"

#include <assert.h>
#include <stdlib.h>

int client_fd;
int server_fd;

static void test_setup(void) {
	bool res;
	const char* port = "100";
	const char* address = "127.0.0.1";

	res = networking_socket_create_server(&server_fd, port, SOCK_STREAM);
	assert(res);
	res = networking_socket_create_client(&client_fd, address, port, SOCK_STREAM);
	assert(res);
}

static void test_teardown(void) {
	char buf[1];

	shutdown(server_fd, SHUT_RDWR);
	close(server_fd);
	shutdown(client_fd, SHUT_RDWR);
	close(client_fd);

	assert(read(server_fd, buf, sizeof(buf)) < 0);
	assert(read(client_fd, buf, sizeof(buf)) < 0);
}

MU_TEST(test_read_write) { // NOLINT(readability-function-cognitive-complexity)
	pid_t p;
	char* msg = "hello";

	p = fork();
	if (p == 0) {
		mu_check(networking_io_writen(client_fd, (uint8_t*) msg, strlen(msg)) == true);
		_exit(0);
	} else {
		char buf[5];
		struct sockaddr_storage their_addr;
		socklen_t their_addr_len;
		int conn_fd;
		enum networking_io_read_status ret;
		conn_fd = accept(server_fd, (struct sockaddr*) &their_addr, &their_addr_len);
		mu_check(conn_fd > 0);
		ret = networking_io_readn(conn_fd, (uint8_t*) buf, sizeof(buf));
		mu_check(ret == NETWORKING_IO_READ_ALL);
		mu_check(0 == memcmp(msg, buf, strlen(msg)));
	}
	wait(NULL);
}

MU_TEST(test_read_fail) { // NOLINT(readability-function-cognitive-complexity)
	pid_t p;
	char* msg = "hello";

	p = fork();
	if (p == 0) {
		shutdown(client_fd, SHUT_RDWR);
		close(client_fd);
		_exit(0);
	} else {
		char buf[5];
		struct sockaddr_storage their_addr;
		socklen_t their_addr_len;
		int conn_fd;
		enum networking_io_read_status ret;
		conn_fd = accept(server_fd, (struct sockaddr*) &their_addr, &their_addr_len);
		mu_check(conn_fd > 0);
		ret = networking_io_readn(conn_fd, (uint8_t*) buf, sizeof(buf));
		mu_check(ret == NETWORKING_IO_READ_UNCONDITION);
		mu_check(0 != memcmp(msg, buf, strlen(msg)));
	}
	wait(NULL);
}

MU_TEST_SUITE(test_suite) { // NOLINT(readability-function-cognitive-complexity)
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);

	MU_RUN_TEST(test_read_write);
	MU_RUN_TEST(test_read_fail);
}

int main(void) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return MU_EXIT_CODE;
}

