#include "queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

static int top;
static int maxtop;
static xtask_task** st;

void initQueue(int leaves, int tails, int threads) {
	st = calloc(leaves + tails, sizeof(xtask_task*));
	maxtop = leaves+tails-1;
	top = 0;
}

void freeQueue() {
	free(st);
}

void enqueue(xtask_task* t, int id) {
	// Grab a valid index that could be dequeued
	int mine = __sync_add_and_fetch(&top, 1) % maxtop;
	// Try to add it to the little stack in that slot
	do t->sibling = st[mine];
	while(!__sync_bool_compare_and_swap(&st[mine], t->sibling, t));
}

xtask_task* dequeue(int id) {
	// Grab a valid index that will be filled (by enqueue)
	int mine = __sync_fetch_and_sub(&top, 1) % maxtop;
	while(1) {
		xtask_task* t;
		// Wait for it to have something
		while((t = st[mine]) == NULL) pthread_testcancel();
		// Detach the head from the rest of the mini-stack
		if(__sync_bool_compare_and_swap(&st[mine], t, t->sibling))
			return t;
	}
}
