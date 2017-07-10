#ifndef QUEUE_H_
#define QUEUE_H_

#include "xtask.h"

// Initialize a Queue for the given xtask_config.
// Defaults are still written as 0's.
void* initQueue(xtask_config*);

// Clean up a Queue. Called only after all the work is done.
void freeQueue(void*);

// Push a task-tree into the Queue.
void push(void*, int tid, xtask_task* tt, xtask_task* parent);

// Leaf a task, which had previously been on the Queue.
// Returns whether to kill all the workers now.
int leaf(void*, int tid, xtask_task* t);

// Pop a single task from the Queue. Should always return a task, and
// provide cancellation points during any spinning.
xtask_task* pop(void*, int tid);

// It should be noted that the only member of xtask_task that must be preserved
// by the Queue is func. All the others may be modified.

#endif /* QUEUE_H_ */
