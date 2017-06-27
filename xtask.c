#include "xtask.h"
#include "queue.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

sem_t finished;		// Signal to main thread that all is done.

struct aftern {
	sem_t sem;
	xtask_task* t;
};

struct aftern_tree {
	int cnt;
	xtask_task sentinal;
	struct aftern ans[];
};

// Count the number of semaphores needed to queue up the given task-tree
static void rcount(xtask_task* tt, int* cnt) {
	if(tt) {
		(*cnt)++;
		for(; tt; tt = tt->sibling)
			rcount(tt->child, cnt);
	}
}

// Build the semaphores and queue up (the leaves of) the given task-tree
static void renqueue(xtask_task* tt, xtask_task* pt, int* ind, struct aftern* ans) {
	if(tt) {
		struct aftern* an = &ans[*ind];
		(*ind)++;

		int cnt = -1;	// So that the last one can't wait
		for(xtask_task* stt = tt; stt; stt = stt->sibling) cnt++;
		sem_init(&an->sem, 0, cnt);
		an->t = pt;

		while(tt) {
			xtask_task* ct = tt->child;
			xtask_task* st = tt->sibling;

			tt->child = (xtask_task*)an;
			if(ct) renqueue(ct, tt, ind, ans);
			else enqueue(tt);

			tt = st;
		}
	}
}

static xtask_task* sent(void* s, xtask_task* d) { return NULL; }

// The two above, put together with a malloc. Returns the aftern array
static struct aftern_tree* rpush(xtask_task* tt, struct aftern* an) {
	int i = 0;
	rcount(tt, &i);

	struct aftern_tree* at = malloc(sizeof(struct aftern_tree)
		+ i*sizeof(struct aftern));
	at->cnt = i;
	at->sentinal = (xtask_task){ sent, 0, (xtask_task*)at, (xtask_task*)an };

	i = 0;
	renqueue(tt, &at->sentinal, &i, at->ans);
	return at;
}

static void empty(void* d) {}

static void* worker(void* vcfg) {
	xtask_config* xc = vcfg;

	int old;
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old);
	void* state = xc->init ? xc->init() : NULL;
	pthread_cleanup_push(xc->dest ? xc->dest : empty, state);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old);

	while(1) {
		xtask_task* t = dequeue();
		if(t) {
			struct aftern* an = (struct aftern*)t->child;
			xtask_task* tt = t->func(state, t);

			#if VALIDATE
			if(tt && (t->fate & XTASK_FATE_LEAF))
				fprintf(stderr, "WARNING: Tailing task with "
					"LEAF fate!\n");
			#endif

			if(tt) rpush(tt, an);
			else while(an && sem_trywait(&an->sem)) {
				if(an->t->func == sent) {
					// Sentinal, cleanup a aftern-tree
					struct aftern_tree* at =
						(struct aftern_tree*)an->t->child;
					an = (struct aftern*)an->t->sibling;
					for(int i=0; i < at->cnt; i++)
						sem_destroy(&at->ans[i].sem);
					free(at);
				} else {
					enqueue(an->t);
					break;
				}
			}

			if(!an) sem_post(&finished);
		} else {
			pthread_testcancel();
			pthread_yield();
		}
	}

	pthread_cleanup_pop(1);
	return NULL;
}

void xtask_run(xtask_task* tt, xtask_config cfg) {
#define def(V, D) if(cfg.V == 0) cfg.V = D;
	def(workers, sysconf(_SC_NPROCESSORS_ONLN))
	def(max_leafing, 20)	// These numbers don't acutally matter, current
	def(max_tailing, 40)	// imp doesn't care.
	def(init, NULL)
	def(dest, NULL)

	// Queue up the first (and only, kind of) task-tree
	initQueue(cfg.max_leafing + cfg.max_tailing);
	rpush(tt, NULL);

	// Setup the ending signal
	sem_init(&finished, 0, 0);

	// Spawn them threads
	pthread_t threads[cfg.workers];
	for(int i=0; i<cfg.workers; i++)
		pthread_create(&threads[i], NULL, worker, &cfg);

	// Wait for everyone to finish up
	sem_wait(&finished);
	for(int i=0; i<cfg.workers; i++) pthread_cancel(threads[i]);
	for(int i=0; i<cfg.workers; i++) pthread_join(threads[i], NULL);

	freeQueue();
}
