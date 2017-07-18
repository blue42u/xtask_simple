#include "queue.h"
#include "common.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

typedef struct {
	int top;
	int maxtop;
	xtask_task** st;
	int heads;
} Q;

void* initQueue(xtask_config* cfg) {
	defaults(cfg, (xtask_config){0, 40, 20, NULL, NULL});
	Q* q = malloc(sizeof(Q));
	q->maxtop = cfg->max_leafing + cfg->max_tailing - 1;
	q->st = calloc(q->maxtop + 1, sizeof(xtask_task*));
	q->top = 0;
	q->heads = 0;
	return q;
}

void freeQueue(void* vq) {
	Q* q = vq;
	free(q->st);
	free(q);
}

static void rpush(Q* q, xtask_task* t, xtask_task* p) {
	if(t->child) {
		// Count the children, using .fate as the after-n counter
		t->fate = 0;
		for(xtask_task* c = t->child; c; c = c->sibling) t->fate++;
		// Set the proper after-n
		xtask_task* ct = t->child;
		t->child = p;
		// Recurse
		while(ct) {
			xtask_task* next = ct->sibling;
			rpush(q, ct, t);
			ct = next;
		}
	} else {
		// Set the results for later
		t->child = p;
		// Grab a valid index to push into
		int mine = __sync_add_and_fetch(&q->top, 1) % q->maxtop;
		if(mine < 0) mine += q->maxtop;
		// Push it into the stask at our index
		do t->sibling = q->st[mine];
		while(!__sync_bool_compare_and_swap(&q->st[mine], t->sibling, t));
	}
}

void push(void* vq, int tid, xtask_task* tt, xtask_task prev) {
	Q* q = vq;
	int extra = prev.func ? -1 : 0;
	for(xtask_task* t=tt; t; t = t->sibling) extra++;
	xtask_task* p = prev.child;

	if(extra > 0) __sync_fetch_and_add(p ? &p->fate : &q->heads, extra);
	while(tt) {
		xtask_task* next = tt->sibling;
		rpush(q, tt, p);
		tt = next;
	}
}

int leaf(void* vq, int tid, xtask_task prev) {
	Q* q = vq;
	xtask_task* t = prev.child;
	if(!t) return __sync_sub_and_fetch(&q->heads, 1) == 0;
	if(__sync_sub_and_fetch(&t->fate, 1) == 0) {
		xtask_task* p = t->child;
		t->child = t->sibling = NULL;
		rpush(q, t, p);
	}
	return 0;
}

xtask_task* pop(void* vq, int tid) {
	Q* q = vq;
	// Grab a valid index to pop from
	int mine = __sync_fetch_and_sub(&q->top, 1) % q->maxtop;
	if(mine < 0) mine += q->maxtop;

	xtask_task* t = NULL;
	do {
		pthread_testcancel();
		// Try and pop from the stack at our index
		do t = q->st[mine];
		while(t && !__sync_bool_compare_and_swap(&q->st[mine],
			t, t->sibling));
	} while(!t);
	return t;
}
