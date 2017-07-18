#include "queue.h"
#include "common.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

typedef struct {
	xtask_task* head;
	xtask_task* second;
	int heads;
} Q;

void* initQueue(xtask_config* cfg) {
	Q* q = malloc(sizeof(Q));
	*q = (Q){NULL, NULL, 0};
	return q;
}

void freeQueue(void* vq) {
	free(vq);
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
		// Set the after-n
		t->child = p;
		// Push it onto the stack
		do t->sibling = q->second;
		while(!__sync_bool_compare_and_swap(&q->second, t->sibling, t));
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
	xtask_task* t;
	do {
		pthread_testcancel();
		// Remove the head, claiming our prize
		do t = q->head;
		while(!__sync_bool_compare_and_swap(&q->head, t, NULL));
		// Move a new item down on second
		xtask_task* s;
		do s = q->second;
		while(s && !__sync_bool_compare_and_swap(&q->second, s, s->sibling));
		// Replace the head with the item moved down
		while(!__sync_bool_compare_and_swap(&q->head, NULL, s));
	} while(!t);
	return t;
}
