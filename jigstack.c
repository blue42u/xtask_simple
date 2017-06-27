#include "queue.h"
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

static int workers;
static xtask_task** heads;
static sem_t* locks;

void initQueue(int leafs, int tails, int threads) {
	workers = threads;
	heads = calloc(threads, sizeof(xtask_task*));
	locks = malloc(threads*sizeof(sem_t));
	for(int i=0; i<threads; i++)
		sem_init(&locks[i], 0, 1);
}

void freeQueue() {
	free(heads);
	for(int i=0; i<workers; i++)
		sem_destroy(&locks[i]);
	free(locks);
}

void enqueue(xtask_task* t, int id) {
	// We always put the new task behind us, so we have a long ways to get
	// to it again. Why? Because I think it might work. Not well, but work.
	id++;
	if(id == workers) id = 0;
	sem_wait(&locks[id]);
	t->sibling = heads[id];
	heads[id] = t;
	sem_post(&locks[id]);
}

xtask_task* dequeue(int id) {
	// Walk our way around the rim, fitting in with the jig.
	xtask_task* t;
	for(int i=id; i >= 0; i--) {
		sem_wait(&locks[i]);
		t = heads[i];
		if(t) heads[i] = t->sibling;
		sem_post(&locks[i]);
		if(t) return t;
	}
	for(int i=workers-1; i > id; i--) {
		sem_wait(&locks[i]);
		t = heads[i];
		if(t) heads[i] = t->sibling;
		sem_post(&locks[i]);
		if(t) return t;
	}
	return NULL;
}
