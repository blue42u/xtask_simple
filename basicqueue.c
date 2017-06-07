#include "basicqueue.h"

void InitBasicQueue(int size)
{
	CIRCLEQ_INIT(&head);
	CIRCLEQ_INIT(&rq_head);
	pthread_mutex_init(&lock, NULL);
}

void BasicEnqueue(int i)
{
	struct basicentry* n = malloc(sizeof(struct basicentry));
	n->elem = i;
	pthread_mutex_lock(&lock);
	CIRCLEQ_INSERT_TAIL(&head, n, entries);
	pthread_mutex_unlock(&lock);
#ifdef VERBOSE
	printf("Added %d to the queue\n", n->elem);
#endif
}

int BasicDequeue()
{
	// Remove a number from the queue
	struct basicentry *n;
	//Keep doing this until we dequeue an actual number, which also ensures queue is not empty for a successful dequeue.
	do
	{
		pthread_mutex_lock(&lock);
		n = CIRCLEQ_FIRST(&head);
		CIRCLEQ_REMOVE(&head, head.cqh_first, entries);
		pthread_mutex_unlock(&lock);
		sched_yield();
	}
	while(n->elem == 0);
#ifdef VERBOSE
	printf("Removed %d from the queue\n", n->elem);
#endif
	int r = n->elem;
	free(n);
	return r;
}

void BasicEnqueue_rq(int i)
{
	struct basicentry* n = malloc(sizeof(struct basicentry));
	n->elem = i;
	pthread_mutex_lock(&lock);
	CIRCLEQ_INSERT_TAIL(&rq_head, n, entries);
	pthread_mutex_unlock(&lock);
#ifdef VERBOSE
	printf("Added %d to the result queue\n", n->elem);
#endif
}

int BasicDequeue_rq()
{
	// Remove a number from the queue
	struct basicentry *n;
	do
	{
		pthread_mutex_lock(&lock);
		n = CIRCLEQ_FIRST(&rq_head);
		CIRCLEQ_REMOVE(&rq_head, rq_head.cqh_first, entries);
		pthread_mutex_unlock(&lock);
		sched_yield();
	}
	while(n->elem == 0);
#ifdef VERBOSE
	printf("Removed %d from the result queue\n", n->elem);
#endif
	int r = n->elem;
	free(n);
	return r;
}
