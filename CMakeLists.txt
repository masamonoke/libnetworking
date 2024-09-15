cmake_minimum_required(VERSION 3.23.2)

set(NAME networking)
project(${NAME})

include_directories(include)

add_library(${PROJECT_NAME}
	src/io.c
	src/socket.c
	src/server_poll.c
	src/server_pthread.c
	src/server_select.c
	src/thread_pool.c
	src/server.c
)

add_subdirectory(test)
enable_testing()

add_test(NAME sock COMMAND sock_test)
add_test(NAME io COMMAND io_test)
add_test(NAME server_poll COMMAND server_poll_test)
add_test(NAME server_select COMMAND server_select_test)
add_test(NAME server COMMAND server_test)
add_test(NAME server_pthread COMMAND server_pthread_test)

if (NOT MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE -Waddress -Wall -Warray-bounds -Wbool-operation -Wchar-subscripts -Wcomment
		-Wmisleading-indentation -Wparentheses -Wextra -pedantic -Wstrict-prototypes -Wshadow -Wconversion -Wvla -Wdouble-promotion -Wmissing-noreturn
		-Wmissing-format-attribute -Wmissing-prototypes -Wunused-result)
endif()

if (${CMAKE_BUILD_TYPE} MATCHES Debug)
	if (NOT WIN32 AND NOT APPLE)
		target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined,null,bounds,leak)
	endif()
	if (APPLE)
		target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined,null,bounds)
	endif()
endif()
