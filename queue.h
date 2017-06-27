#ifndef QUEUE_H_
#define QUEUE_H_

#include "xtask.h"

void initQueue(int size);
void freeQueue();

void enqueue(xtask_task*);
xtask_task* dequeue();

#endif /* QUEUE_H_ */
