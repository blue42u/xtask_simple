#include "xtask_api.h"
#include "queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>

struct _xtask_aftern_internal {
	sem_t lock;
	int cnt;
};

// Vars for general state
static int workers;
static pthread_t* threads;
static void* (*sinit)();
static void (*sfree)(void*);

// Vars to inter-worker comms
static sem_t running;
static int complete;
static sem_t lock;

// Forward def
static void* body(void*);

void xtask_setup(void* (*s_init)(), void (*s_free)(void*),
	int queue_size, int works) {

	initQueue(queue_size);

	workers = works;
	threads = malloc(workers * sizeof(pthread_t));
	sinit = s_init;
	sfree = s_free;

	sem_init(&running, 0, workers);
	sem_init(&lock, 0, 1);
	complete = 0;

	for (int t = 0; t < workers; t++)
		pthread_create(&threads[t], NULL, body, NULL);
}

static void notask(void* s, void* d, int isn) {}

void xtask_cleanup() {
	// Ensure one of the workers will now stop spinning.
	enqueue(&(xtask_task_t){ notask, NULL, NULL });

	// Lower the running by one, someone will set complete to 1.
	if(sem_trywait(&running)) complete = 1;

	for (int t = 0; t < workers; t++)
		pthread_join(threads[t], NULL);	// Wait for the threads to die.

	sem_destroy(&running);
	sem_destroy(&lock);
	free(threads);
	freeQueue();
}

void xtask_push(const xtask_task_t* task) {
	enqueue(task);
}

xtask_aftern_t xtask_aftern_create(int n) {
	xtask_aftern_t an = malloc(sizeof(struct _xtask_aftern_internal));
	sem_init(&an->lock, 0, 1);
	an->cnt = n;
	return an;
}

static void* body(void* dummy) {
	void* state = NULL;
	if(sinit) state = sinit();

	xtask_task_t task;
	int spin = 0;
	while(!complete) {
		if(dequeue(&task)) {
			if(spin) sem_post(&running);
			spin = 0;

			int isn = 0;
			if(task.aftern) {
				sem_wait(&task.aftern->lock);
				task.aftern->cnt--;
				if(task.aftern->cnt == 0) {
					isn = 1;
					sem_destroy(&task.aftern->lock);
					free(task.aftern);
				} else sem_post(&task.aftern->lock);
			}
			task.task(state, task.data, isn);
		} else {
			if(!spin) if(sem_trywait(&running)) complete = 1;
			spin = 1;

			sched_yield();
		}
	}

	if(sfree) sfree(state);
	return NULL;
}
