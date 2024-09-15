#include "networking/io.h"

#include <sys/socket.h>
#include <errno.h>

enum networking_io_read_status networking_io_readn(int fd, uint8_t* buf, size_t n) {
	ssize_t rv;
	size_t nleft;

	nleft = n;
	while (nleft > 0) {
		rv = recv(fd, buf, n, 0);
		if (rv < 0) {
			if (errno == EINTR) {
				rv = 0;
			} else {
				return NETWORKING_IO_READ_FAIL;
			}
		} else if (rv == 0) {
			break;
		}

		nleft -= (size_t) rv;
		buf += rv;
	}

	return (nleft == 0) ? NETWORKING_IO_READ_ALL : NETWORKING_IO_READ_UNCONDITION;
}

bool networking_io_writen(int fd, const uint8_t* buf, size_t n) {
	size_t nleft;
	ssize_t written;

	nleft = n;
	while (nleft > 0) {
		written = send(fd, buf, n, 0);
		if (written <= 0) {
			if (written < 0 && errno == EINTR) {
				written = 0;
			} else {
				return false;
			}
		}

		nleft -= (size_t) written;
		buf += written;
	}

	return true;
}
