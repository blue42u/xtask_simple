#include "xtask_api.h"
#include "queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

struct _xtask_aftern_internal {
	sem_t lock;
	int cnt;
	xtask_task_t task;
};

// Vars for general state
static int workers;
static pthread_t* threads;
static void* (*sinit)();
static void (*sfree)(void*);

// Vars to inter-worker comms
static int complete;

// Forward def
static void* body(void*);

void xtask_setup(void* (*s_init)(), void (*s_free)(void*),
	int queue_size, int works) {

	initQueue(queue_size);

	workers = works;
	threads = malloc(workers * sizeof(pthread_t));
	sinit = s_init;
	sfree = s_free;

	complete = 0;

	for (int t = 0; t < workers; t++)
		pthread_create(&threads[t], NULL, body, NULL);
}

static int final(void* s, void* d, xtask_aftern_t t) {
	complete = 1;
	return 1;
}

const xtask_task_t xtask_final = { final, NULL, NULL };

void xtask_cleanup() {
	for (int t = 0; t < workers; t++)
		pthread_join(threads[t], NULL);	// Wait for the threads to die.

	free(threads);
	freeQueue();
}

void xtask_push(const xtask_task_t* task) {
	enqueue(task);
}

xtask_aftern_t xtask_aftern_create(int n, const xtask_task_t* t) {
	xtask_aftern_t an = malloc(sizeof(struct _xtask_aftern_internal));
	sem_init(&an->lock, 0, 1);
	an->cnt = n;
	an->task = *t;
	return an;
}

static void* body(void* dummy) {
	void* state = NULL;
	if(sinit) state = sinit();

	xtask_task_t task;
	while(!complete) {
		if(dequeue(&task)) {
			int tail = 1;
			if(task.task)
				tail = task.task(state, task.data, task.aftern);
			if(tail && task.aftern) {
				sem_wait(&task.aftern->lock);
				task.aftern->cnt--;
				if(task.aftern->cnt == 0) {
					xtask_push(&task.aftern->task);
					sem_destroy(&task.aftern->lock);
					free(task.aftern);
				} else sem_post(&task.aftern->lock);
			}
		} else sched_yield();
	}

	if(sfree) sfree(state);
	return NULL;
}
