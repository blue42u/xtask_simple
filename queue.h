#ifndef QUEUE_H_
#define QUEUE_H_

#include "xtask.h"

void initQueue(int leafs, int tails, int threads);
void freeQueue();

void enqueue(xtask_task*, int tid);
xtask_task* dequeue(int tid);

#endif /* QUEUE_H_ */
