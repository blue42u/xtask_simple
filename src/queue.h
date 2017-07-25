#ifndef QUEUE_H_
#define QUEUE_H_

#include "xtask.h"

// Initialize a Queue for the given xtask_config.
// Defaults are still written as 0's by this point.
void* initQueue(xtask_config*);

// Clean up a Queue. Called only after all the work is done.
void freeQueue(void*);

// Get ready to push a task-tree of the given size into the Queue.
// If this is the result of tailing, prev will point to a copy of the task, else
// it will be NULL. <tasks> is the total size of the tree, while <leaves> is the
// number of leaves at the bottom of the tree, and <roots> is the number of trees.
// The return value is passed to the other *push functions.
void* prepush(void*, int tid, xtask_task* prev, int roots, int tasks, int leaves);

// Prepare a non-leaf for pushing onto the queue, when its dependencies are met.
void midpush(void* q, void* pp, int tid, xtask_task* t, xtask_task* p, int numc);

// Push a leaf onto the Queue. It may be popped before the push is complete.
void leafpush(void* q, void* pp, int tid, xtask_task* t, xtask_task* p);

// Cleanup the structure used by the push.
void postpush(void* q, void* pp);

// Leaf a task, which had previously been on the Queue.
// Returns whether to kill all the workers now.
int leaf(void*, int tid, xtask_task prev);

// Pop a single task from the Queue. Should always return a task, and
// provide cancellation points during any spinning.
xtask_task* pop(void*, int tid);

// It should be noted that the only member of xtask_task that must be preserved
// by the Queue is func. All the others may be modified.

#endif /* QUEUE_H_ */
