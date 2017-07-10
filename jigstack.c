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

static void rpush(Q* q, int id, xtask_task* tt, aftern* an, aftern* ans, int* ind) {
	if(tt->child) {
		int cnt = 0;
		for(xtask_task* c = tt->child; c; c = c->sibling) cnt++;

		aftern* myan = &ans[*ind++];
		sem_init(&myan->cnt, 0, cnt-1);
		myan->size = 0;
		myan->t = tt;

		xtask_task* c = tt->child;
		tt->child = an;
		while(c) {
			xtask_task* next = c->sibling;
			rpush(q, id, c, myan, ans, ind);
			c = next;
		}
	} else {
		id = (id+1);
		if(id == q->size) id = 0;

		tt->child = an;

		sem_wait(&q->locks[id]);
		tt->sibling = q->heads[id];
		q->heads[id] = tt;
		sem_post(&q->locks[id]);
	}
}

void push(void* vq, int tid, xtask_task* tt, xtask_task prev) {
	int size = rcount(tt) + 1;
	aftern* ans = malloc(size*sizeof(aftern));

	int cnt = 0;
	for(xtask_task* s = tt; s; s = s->sibling) cnt++;
	sem_init(&ans[0].cnt, 0, cnt-1);
	ans[0].size = size;
	ans[0].an = prev.child;

	int ind = 1;
	while(tt) {
		xtask_task* next = tt->sibling;
		rpush(vq, tid, tt, &ans[0], ans, &ind);
		tt = next;
	}
}

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
			aftern* next = an->t->child;
			an->t->sibling = an->t->child = NULL;
			rpush(vq, tid, an->t, next, NULL, NULL);
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
