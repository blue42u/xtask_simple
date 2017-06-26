#include "queue.h"
#include <semaphore.h>
#include <stdlib.h>

typedef struct node {
	struct node* next;
	xtask_task_t task;
} node;

static node* head;
static sem_t lock;

void initQueue(int size) {
	head = NULL;
	sem_init(&lock, 0, 1);
}

void freeQueue() {
	sem_wait(&lock);
	while(head != NULL) {
		node* next = head->next;
		free(head);
		head = next;
	}
	sem_destroy(&lock);
}

void enqueue(const xtask_task_t* t) {
	node* n = malloc(sizeof(node));
	n->task = *t;

	sem_wait(&lock);
	n->next = head;
	head = n;
	sem_post(&lock);
}

int dequeue(xtask_task_t* o) {
	node* n = NULL;

	sem_wait(&lock);
	if(head) {
		n = head;
		head = head->next;
	}
	sem_post(&lock);

	if(n) {
		*o = n->task;
		free(n);
		return 1;
	} else return 0;
}
