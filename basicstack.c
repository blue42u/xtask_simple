#include "queue.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

#define COUNT 0

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// We reuse the sibling pointers to form the stack.
static xtask_task* head;
#if COUNT
static unsigned long long taskcnt;	// Counter for tasks
#endif

void initQueue(int size) {
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);

	// We have fast lock-unlock cycles, so allow things to speed up.
	pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

	pthread_mutex_init(&lock, &attr);
	head = NULL;

	pthread_mutexattr_destroy(&attr);
}

void freeQueue() {
	// We assume we only free the queue when it's empty, so don't bother with
	// freeing the queue. Besides, its the user's job.
	pthread_mutex_destroy(&lock);
#if COUNT
	fprintf(stderr, "Task Count: %lld\n", taskcnt);
#endif
}

void enqueue(xtask_task* t) {
	pthread_mutex_lock(&lock);
	t->sibling = head;
	head = t;
#if COUNT
	taskcnt++;
#endif
	pthread_mutex_unlock(&lock);
}

xtask_task* dequeue() {
	xtask_task* t;
	pthread_mutex_lock(&lock);
	t = head;
	if(t) head = t->sibling;
	pthread_mutex_unlock(&lock);
	return t;
}
