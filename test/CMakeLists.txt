cmake_minimum_required(VERSION 3.23.2)

set(NAME tests)
project(${NAME})

set(CMAKE_C_STANDARD 17)

include_directories(deps/minunit)
include_directories(../include)

OPTION(DEFINE_TCP "Use TCP in socket test" ON)
IF(DEFINE_TCP)
	ADD_DEFINITIONS(-DTCP)
ENDIF(DEFINE_TCP)

add_executable(sock_test
	../src/socket.c
	deps/minunit/minunit.h
	sock_test.c
)

add_executable(io_test
	../src/io.c
	../src/socket.c
	deps/minunit/minunit.h
	io_test.c
)

add_executable(server_poll_test
	deps/minunit/minunit.h
	../src/server_poll.c
	../src/socket.c
	server_poll_test.c
)

add_executable(server_select_test
	deps/minunit/minunit.h
	../src/server_select.c
	../src/socket.c
	server_select_test.c
)

add_executable(server_pthread_test
	deps/minunit/minunit.h
	../src/server_pthread.c
	../src/socket.c
	../src/thread_pool.c
	server_pthread_test.c
)

# WARN: if new server module created then you need to append it here to test
add_executable(server_test
	deps/minunit/minunit.h
	../src/server.c
	../src/socket.c
	../src/server_select.c
	../src/server_poll.c
	../src/server_pthread.c
	../src/thread_pool.c
	server_test.c
)

set(TEST_TARGETS
	sock_test
	io_test
	server_poll_test
	server_select_test
	server_test
	server_pthread_test
)

foreach(target IN LISTS TEST_TARGETS)
	if (NOT MSVC)
		target_compile_options(${target} PRIVATE -Waddress -Wall -Warray-bounds -Wbool-operation -Wchar-subscripts -Wcomment
			-Wmisleading-indentation -Wparentheses -Wextra -pedantic -Wstrict-prototypes -Wshadow -Wconversion -Wvla -Wdouble-promotion -Wmissing-noreturn
			-Wmissing-format-attribute -Wmissing-prototypes -Wunused-result)
	endif()

	if (${CMAKE_BUILD_TYPE} MATCHES Debug)
		if (NOT WIN32 AND NOT APPLE)
			target_link_options(${target} PRIVATE -fsanitize=address,undefined,null,bounds,leak)
		endif()
		if (APPLE)
			target_link_options(${target} PRIVATE -fsanitize=address,undefined,null,bounds)
		endif()
	endif()
endforeach(target IN LISTS TEST_TARGETS)
