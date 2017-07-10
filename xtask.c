#include "xtask.h"
#include "queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t finished;		// Signal to main thread that all is done.

static void empty(void* d) {}

struct workerdata {
	int id;
	xtask_config* c;
	void* q;
};

static void* worker(void* vcfg) {
	struct workerdata* wd = vcfg;

	int old;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
	void* state = wd->c->init ? wd->c->init() : NULL;
	pthread_cleanup_push(wd->c->dest ? wd->c->dest : empty, state);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);

	while(1) {
		xtask_task* t = pop(wd->q, wd->id);
		xtask_task orig = *t;
		xtask_task* tt = t->func(state, t);

		if(tt) {
			tt->sibling = NULL;
			push(wd->q, wd->id, tt, &orig);
		} else if(leaf(wd->q, wd->id, &orig)) sem_post(&finished);
	}

	pthread_cleanup_pop(1);
	return NULL;
}

void xtask_run(void* tt, xtask_config cfg) {
	// Queue up the first (and only, kind of) task-tree
	void* q = initQueue(&cfg);
	((xtask_task*)tt)->sibling = NULL;
	push(q, 0, tt, NULL);

	// Setup the ending signal
	sem_init(&finished, 0, 0);

	// Spawn them threads
	pthread_t threads[cfg.workers];
	struct workerdata tds[cfg.workers];
	for(int i=0; i<cfg.workers; i++) {
		tds[i] = (struct workerdata){ i, &cfg, q };
		pthread_create(&threads[i], NULL, worker, &tds[i]);
	}

	// Wait for everyone to finish up
	sem_wait(&finished);
	for(int i=0; i<cfg.workers; i++) pthread_cancel(threads[i]);
	for(int i=0; i<cfg.workers; i++) pthread_join(threads[i], NULL);

	freeQueue(q);
}
