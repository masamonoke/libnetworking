#include "networking/utils/thread_pool.h"

#include <pthread.h>
#include <stdlib.h>

typedef struct tp_work {
	thread_callback_t f;
	void* args;
	struct tp_work* next;
} tp_work_t;

struct thread_pool {
	tp_work_t* head;
	tp_work_t* tail;
	pthread_mutex_t work_mutex;
	pthread_cond_t have_work_cond;
	pthread_cond_t no_active_worker_cond;
	size_t working_num;
	size_t thread_num;
	bool stop;
};


static tp_work_t* work_new(thread_callback_t f, void* args);
static void work_delete(tp_work_t* work);
static tp_work_t* work_pop(thread_pool_t* tp);
static void* worker(void* args);

thread_pool_t* thread_pool_new(size_t num) {
	thread_pool_t* tp;
	pthread_t thread;
	size_t i;

	if (num == 0) {
		num = 2;
	}

	tp = calloc(1, sizeof(*tp));
	tp->thread_num = num;

	pthread_mutex_init(&tp->work_mutex, NULL);
	pthread_cond_init(&tp->have_work_cond, NULL);
	pthread_cond_init(&tp->no_active_worker_cond, NULL);

	tp->head = NULL;
	tp->tail = NULL;

	for (i = 0; i < num; i++) {
		pthread_create(&thread, NULL, worker, tp);
		pthread_detach(thread);
	}

	return tp;
}

void thread_pool_delete(thread_pool_t* tp) {
	tp_work_t* work;
	tp_work_t* tmp;

	if (tp == NULL) {
		return;
	}

	pthread_mutex_lock(&tp->work_mutex);
	work = tp->head;
	while (work != NULL) {
		tmp = work->next;
		work_delete(work);
		work = tmp;
	}

	tp->head = NULL;
	tp->stop = true;

	pthread_cond_broadcast(&tp->have_work_cond); // to unlock condition and proceed to stop 'if'
	pthread_mutex_unlock(&tp->work_mutex);

	// TODO: probably make option to add timer
	thread_pool_wait(tp);

	pthread_mutex_destroy(&tp->work_mutex);
	pthread_cond_destroy(&tp->have_work_cond);
	pthread_cond_destroy(&tp->no_active_worker_cond);

	free(tp);
}

bool thread_pool_add_work(thread_pool_t* tp, thread_callback_t f, void* args) {
	tp_work_t* work;

	if (tp == NULL) {
		return false;
	}

	work = work_new(f, args);
	if (work == NULL) {
		return false;
	}

	pthread_mutex_lock(&tp->work_mutex);
	if (tp->head == NULL) {
		tp->head = work;
		tp->tail = work;
	} else {
		tp->tail->next = work;
		tp->tail = work;
	}

	pthread_cond_broadcast(&tp->have_work_cond);
	pthread_mutex_unlock(&tp->work_mutex);

	return true;
}

void thread_pool_wait(thread_pool_t* tp) {
	if (tp == NULL) {
		return;
	}

	pthread_mutex_lock(&tp->work_mutex);
	while (true) {
		if (tp->head != NULL || (!tp->stop && tp->working_num != 0) || (tp->stop && tp->thread_num != 0)) {
			pthread_cond_wait(&tp->no_active_worker_cond, &tp->work_mutex);
		} else {
			break;
		}
	}
	pthread_mutex_unlock(&tp->work_mutex);
}

static tp_work_t* work_new(thread_callback_t f, void* args) {
	tp_work_t* work;

	if (f == NULL) {
		return NULL;
	}

	work = malloc(sizeof(*work));
	work->f = f;
	work->args = args;
	work->next = NULL;
	return work;
}

static void work_delete(tp_work_t* work) {
	free(work);
}

static tp_work_t* work_pop(thread_pool_t* tp) {
	tp_work_t* work;

	if (tp == NULL) {
		return NULL;
	}

	work = tp->head;
	if (work == NULL) {
		return NULL;
	}

	if (work->next == NULL) {
		tp->head = NULL;
		tp->tail = NULL;
	} else {
		tp->head = work->next;
	}

	return work;
}

static void* worker(void* args) {
	thread_pool_t* tp = args;
	tp_work_t* work;

	while (true) {
		pthread_mutex_lock(&tp->work_mutex);

		while (tp->head == NULL && !tp->stop) {
			pthread_cond_wait(&tp->have_work_cond, &tp->work_mutex); // automatically relock mutex
		}

		if (tp->stop) {
			break;
		}

		work = work_pop(tp);
		tp->working_num++;
		pthread_mutex_unlock(&tp->work_mutex);

		if (work != NULL) {
			work->f(work->args);
			work_delete(work);
		}

		pthread_mutex_lock(&tp->work_mutex);
		tp->working_num--;
		if (!tp->stop && tp->working_num == 0 && tp->head == NULL) {
			pthread_cond_signal(&tp->no_active_worker_cond);
		}
		pthread_mutex_unlock(&tp->work_mutex);
	}

	tp->thread_num--;
	pthread_cond_signal(&tp->no_active_worker_cond);
	pthread_mutex_unlock(&tp->work_mutex);

	return NULL;
}
