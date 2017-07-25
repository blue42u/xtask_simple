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

unsigned long long cnt = 0;

void printout() {
	fprintf(stderr, "TASKCOUNT %lld\n", cnt);
}

void* initQueue(xtask_config* cfg) {
	cfg->workers = 1;	// We only support a single thread.
	queue* q = malloc(sizeof(queue));
	q->head = NULL;
	q->cnt = 0;
	if(cnt == 0) atexit(printout);
	return q;
}

void freeQueue(void* vq) {
	queue* q = vq;
	cnt += q->cnt;
	free(q);
}

void* prepush(void* q, int i, xtask_task* p, int r, int t, int l) {
	return p ? p->child : NULL;
}
void postpush(void* q, void* pp) {}

void midpush(void* vq, void* vpp, int id, xtask_task* t, xtask_task* p, int nc) {
	t->fate = nc;
	t->child = p ? p : vpp;
}

void leafpush(void* vq, void* vpp, int id, xtask_task* t, xtask_task* p) {
	queue* q = vq;
	q->cnt++;
	t->sibling = q->head;
	q->head = t;
	t->child = p ? p : vpp;
}

int leaf(void* vq, int tid, xtask_task prev) {
	xtask_task* t = prev.child;
	if(!t) return 1;
	t->fate--;
	if(t->fate > 0) return 0;
	xtask_task* p = t->child;
	t->child = NULL;
	t->sibling = NULL;
	leafpush(vq, NULL, 0, t, p);
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
