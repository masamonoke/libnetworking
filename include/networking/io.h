#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

enum networking_io_read_status {
	NETWORKING_IO_READ_ALL,
	NETWORKING_IO_READ_UNCONDITION,
	NETWORKING_IO_READ_FAIL
};

// Reading exactly n bytes from fd and it will block until n bytes is read
// Returns NETWORKING_IO_READ_ALL if no errors occured and n bytes read.
// Returns NETWORKING_IO_READ_UNCONDITION if sudden EOF happend and not n bytes were read or read 0 bytes.
// Returns NETWORKING_IO_READ_FAIL in case of error other than EINTR or wrong fd (e.g. fd closed while reading or some other system error)
__attribute__((warn_unused_result, nonnull(2)))
enum networking_io_read_status networking_io_readn(int fd, uint8_t* buf, size_t n);

// Writing n bytes to fd.
// Returns true if no errors occured and n bytes written.
// Returns false in case of error other than EINTR or wrong fd (e.g. closed).
__attribute__((warn_unused_result, nonnull(2)))
bool networking_io_writen(int fd, const uint8_t* buf, size_t n);
