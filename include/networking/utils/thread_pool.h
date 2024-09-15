#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct thread_pool thread_pool_t;
typedef void (*thread_callback_t) (void* args);

thread_pool_t* thread_pool_new(size_t num);

void thread_pool_delete(thread_pool_t* tp);

bool thread_pool_add_work(thread_pool_t* tp, thread_callback_t f, void* args);

void thread_pool_wait(thread_pool_t* tp);
