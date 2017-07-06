#include "queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

static int top;
static xtask_task** st;

void initQueue(int leaves, int tails, int threads) {
	st = calloc(leaves+tails+threads, sizeof(xtask_task*));
	top = threads;
}

void freeQueue() {
	free(st);
}

void enqueue(xtask_task* t, int id) {
	int mine = __sync_add_and_fetch(&top, 1);
	while(!__sync_bool_compare_and_swap(&st[mine], NULL, t))
		pthread_testcancel();
}

xtask_task* dequeue(int id) {
	int mine = __sync_fetch_and_sub(&top, 1);
	while(1) {
		xtask_task* t;
		while((t = st[mine]) == NULL) pthread_testcancel();
		if(__sync_bool_compare_and_swap(&st[mine], t, NULL))
			return t;
	}
}
