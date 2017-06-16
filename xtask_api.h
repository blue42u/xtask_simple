/*
 * xtask_api.h
 *
 *  Created on: Jun 2, 2017
 *      Author: pnookala
 */

#ifndef XTASK_API_H_
#define XTASK_API_H_

// After-n primitive, only triggers on the nth usage.
typedef struct _xtask_aftern_internal* xtask_aftern_t;

// Task structure, to aid in pushing and such.
// <task> is called with the <state> of the worker (from `init_state`), <data>,
// and the <aftern> it could trigger. <aftern> is triggered only if <task>
// returns a true value, otherwise it is assumed that a task will be pushed with
// <aftern> as its aftern.
typedef struct {
	int (*task)(void* state, void* data, xtask_aftern_t);
	void* data;
	xtask_aftern_t aftern;
} xtask_task_t;

// The final task. Once this task is pushed, any tasks that do not happen-before
// this task may not be completed, but must be pushed at some point.
const xtask_task_t xtask_final;

// Setup the XTask system. Should only be called once.
void xtask_setup(void* (*init_state)(), void (*free_state)(void*),
	int queue_size, int workers);

// Cleanup the system, waits for the finisher to activate.
void xtask_cleanup();

// Create a new after-n primitive with the given n. Should be used n times,
// since it is destroyed after the nth usage. After the nth execution has
// completed, <task> is queued for execution.
xtask_aftern_t xtask_aftern_create(int n, const xtask_task_t* task);

// Push a new copy of a task into the system, which will be executed async.
void xtask_push(const xtask_task_t* task);

#endif /* XTASK_API_H_ */
