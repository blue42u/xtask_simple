#ifndef XTASK_H_
#define XTASK_H_

// This is filled by the user, to specify various tuning-style parameters for
// execution. Default for any value is 0, so {} is all defaults
typedef struct xtask_config {
	int workers;		// Number of workers to use.
	int max_leafing;	// Max number of leafing tasks to keep at once.
	int max_tailing;	// Max number of tailing tasks to keep at once.

	void* (*init)();	// Initializer and finalizer for custom states.
	void  (*dest)(void*);	// Every task is passed one, which is unique
				// for the duration of the task's execution.
} xtask_config;

// Bitmask specifying the fate of a task. If any flag is set, it must be correct.
typedef enum xtask_fate {
	XTASK_FATE_LEAF = 1 << 0,	// The task will leaf, rather than tail.
} xtask_fate;

// The specification for a task, which may be part of a task-tree. Children must
// complete execution before the parents may begin, thus forming task dependencies.

// Parents point to the head of their children, which form a linked list between
// siblings. It is recommended that the parent task frees the entire tree.

// <fate> defines the fate of the task, which may provide some performance
// benefits. Descriptions of the different fates are above.

// <child> and <sibling> must be castable to a xtask_task*. Thus, if the user has
// them point to a custom structure, an xtask_task must be the first member.
// <data> will be the pointer to the task it resides in.

// <func> is called with the pointer of the task that contains it. The returned
// task is the head of a task-tree (which forms the tail), or NULL (for leaves).
// The values in the structure may change before <func> is executed.
typedef struct xtask_task xtask_task;
struct xtask_task {
	void* (*func)(void* state, void* data);
	int fate;
	void* child;
	void* sibling;
};

// Using the given config, run the specified task-tree. Does not return before
// the head task and all tailed tasks afterwards have completed execution.
// <tt> must be castable to an xtask_task*.
void xtask_run(void* tt, xtask_config);

#endif /* XTASK_H_ */
