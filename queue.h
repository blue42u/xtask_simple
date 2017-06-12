#ifndef QUEUE_H_
#define QUEUE_H_

#include "xtask_api.h"

void initQueue(int size);
void freeQueue();

void enqueue(const xtask_task_t*);
int dequeue(xtask_task_t*);

#endif /* QUEUE_H_ */
