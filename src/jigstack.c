#include "queue.h"
#include "common.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>
#include <stdbool.h>

typedef struct {
	int size;
	xtask_task** heads;
	sem_t* locks;
} Q;

void* initQueue(xtask_config* cfg) {
	Q* q = malloc(sizeof(Q));
	q->size = cfg->workers;
	q->heads = calloc(q->size, sizeof(xtask_task*));
	q->locks = malloc(q->size*sizeof(sem_t));
	for(int i=0; i < q->size; i++)
		sem_init(&q->locks[i], 0, 1);
	return q;
}

void freeQueue(void* vq) {
	Q* q = vq;
	free(q->heads);
	for(int i=0; i < q->size; i++)
		sem_destroy(&q->locks[i]);
	free(q->locks);
	free(q);
}

typedef struct aftern {
	sem_t cnt;
	int size;
	union {
		xtask_task* t;
		struct aftern* an;
	};
} aftern;

typedef struct {
	aftern* ans;
	int ind;
} pushstuff;

void* prepush(void* vq, int id, xtask_task* prev, int nr, int nt, int nl) {
	pushstuff* ps = malloc(sizeof(pushstuff));
	ps->ans = malloc((nt-nl+1)*sizeof(aftern));
	ps->ind = 1;

	sem_init(&ps->ans[0].cnt, 0, nr-1);
	ps->ans[0].size = nt-nl+1;
	ps->ans[0].an = prev ? prev->child : NULL;

	return ps;
}

void midpush(void* vq, void* vps, int id, xtask_task* t, xtask_task* p, int nc) {
	pushstuff* ps = vps;
	aftern* myan = &ps->ans[ps->ind++];
	sem_init(&myan->cnt, 0, nc-1);
	myan->size = 0;
	myan->t = t;

	t->child = p ? p->sibling : &ps->ans[0];
	t->sibling = myan;
}

void leafpush(void* vq, void* vps, int id, xtask_task* t, xtask_task* p) {
	Q* q = vq;
	pushstuff* ps = vps;

	if((++id) == q->size) id = 0;
	t->child = p ? p->sibling : &ps->ans[0];

	sem_wait(&q->locks[id]);
	t->sibling = q->heads[id];
	q->heads[id] = t;
	sem_post(&q->locks[id]);
}

void postpush(void* vq, void* vps) { free(vps); }

int leaf(void* vq, int tid, xtask_task prev) {
	aftern* an = prev.child;
	while(an) {
		if(!sem_trywait(&an->cnt)) return 0;
		if(an->size > 0) {
			aftern* next = an->an;
			for(int i=0; i < an->size; i++)
				sem_destroy(&an[i].cnt);
			free(an);
			an = next;
		} else {
			xtask_task d = {NULL, 0, NULL, an->t->child};
			leafpush(vq, NULL, tid, an->t, &d);
			return 0;
		}
	}
	return 1;
}

xtask_task* pop(void* vq, int id) {
	Q* q = vq;
	// Walk our way around the rim, fitting in with the jig.
	xtask_task* t;
	while(1) {
		for(int i=id; i >= 0; i--) {
			sem_wait(&q->locks[i]);
			t = q->heads[i];
			if(t) q->heads[i] = t->sibling;
			sem_post(&q->locks[i]);
			if(t) return t;
		}
		for(int i=q->size-1; i > id; i--) {
			sem_wait(&q->locks[i]);
			t = q->heads[i];
			if(t) q->heads[i] = t->sibling;
			sem_post(&q->locks[i]);
			if(t) return t;
		}
		pthread_testcancel();
		sched_yield();
	}
}
