#include "queue.h"
#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

typedef struct {
	xtask_task* head;
	unsigned long long cnt;
} queue;

void* initQueue(xtask_config* cfg) {
	cfg->workers = 1;	// We only support a single thread.
	queue* q = malloc(sizeof(queue));
	q->head = NULL;
	q->cnt = 0;
	return q;
}

void freeQueue(void* vq) {
	queue* q = vq;
	fprintf(stderr, "TASKCOUNT %lld\n", q->cnt);
	free(q);
}

static void rpush(queue* q, xtask_task* tt, xtask_task* p) {
	if(tt->child) {
		int cnt = 0;
		for(xtask_task* c = tt->child; c; cnt++) {
			xtask_task* next = c->sibling;
			rpush(q, c, tt);
			c = next;
		}
		tt->fate = cnt;
	} else {
		q->cnt++;
		tt->sibling = q->head;
		q->head = tt;
	}
	tt->child = p;
}

void push(void* vq, int tid, xtask_task* tt, xtask_task prev) {
	while(tt) {
		xtask_task* next = tt->sibling;
		rpush(vq, tt, prev.child);
		tt = next;
	}
}

int leaf(void* vq, int tid, xtask_task prev) {
	xtask_task* t = prev.child;
	if(!t) return 1;
	t->fate--;
	if(t->fate > 0) return 0;
	xtask_task* p = t->child;
	t->child = NULL;
	t->sibling = NULL;
	rpush(vq, t, p);
	return 0;
}

xtask_task* pop(void* vq, int tid) {
	queue* q = vq;
	if(q->head) {
		xtask_task* t = q->head;
		q->head = t->sibling;
		return t;
	} else sleep(1000000);
	fprintf(stderr, "Shouldn't have gotten here!\n");
	return NULL;
}
