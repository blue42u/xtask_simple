#include "xtask.h"
#include "queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

static void empty(void* d) {}

struct workerdata {
	int id;
	xtask_config* c;
	void* q;
	sem_t* finished;
};

static void rcount(xtask_task* tt, int* n, int* l) {
	if(!tt->child) (*l)++;
	(*n)++;
	for(xtask_task* c = tt->child; c; c = c->sibling)
		rcount(c, n, l);
}

static void rpush(void* q, void* pp, int id, xtask_task* tt, xtask_task* p) {
	while(tt) {
		xtask_task* next = tt->sibling;
		if(tt->child) {
			xtask_task* c = tt->child;
			int cnt = 0;
			for(xtask_task* cs = c; cs; cs = cs->sibling) cnt++;
			midpush(q, pp, id, tt, p, cnt);
			rpush(q, pp, id, c, tt);
		} else leafpush(q, pp, id, tt, p);
		tt = next;
	}
}

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
			int r = 0, n = 0, l = 0;
			for(xtask_task* t = tt; t; t = t->sibling, r++)
				rcount(t, &n, &l);
			void* pp = prepush(wd->q, wd->id, &orig, r, n, l);
			rpush(wd->q, pp, wd->id, tt, NULL);
			postpush(wd->q, pp);
		} else if(leaf(wd->q, wd->id, orig)) sem_post(wd->finished);
	}

	pthread_cleanup_pop(1);
	return NULL;
}

void xtask_run(void* tt, xtask_config cfg) {
	void* q = initQueue(&cfg);

	// Queue up the task-tree.
	int r = 0, n = 0, l = 0;
	for(xtask_task* t = tt; t; t = t->sibling, r++)
		rcount(t, &n, &l);
	void* pp = prepush(q, 0, NULL, r, n, l);
	rpush(q, pp, 0, tt, NULL);
	postpush(q, pp);

	// Setup the ending signal
	sem_t finished;
	sem_init(&finished, 0, 0);

	// Spawn them threads
	pthread_t threads[cfg.workers];
	struct workerdata tds[cfg.workers];
	for(int i=0; i<cfg.workers; i++) {
		tds[i] = (struct workerdata){ i, &cfg, q, &finished };
		pthread_create(&threads[i], NULL, worker, &tds[i]);
	}

	// Wait for everyone to finish up
	sem_wait(&finished);
	for(int i=0; i<cfg.workers; i++) pthread_cancel(threads[i]);
	for(int i=0; i<cfg.workers; i++) pthread_join(threads[i], NULL);

	freeQueue(q);
}
