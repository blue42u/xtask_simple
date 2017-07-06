#include "queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

static xtask_task* head;
static unsigned long long taskcnt;

void initQueue(int leafs, int tails, int threads) {
	head = NULL;
	taskcnt = 0;
}

void freeQueue() {
	fprintf(stderr, "TASKCOUNT %lld\n", taskcnt);
}

void enqueue(xtask_task* t, int tid) {
	t->sibling = head;
	head = t;
	taskcnt++;
}

xtask_task* dequeue(int tid) {
	xtask_task* t = head;
	if(!t) while(1) pthread_testcancel();
	head = t->sibling;
	return t;
}
