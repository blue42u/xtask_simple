#include "xtask.h"
#include "xtask_api.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define VALIDATE 1

static void rpush(xtask_task*, xtask_aftern_t);		// Pushes a task-tree.
static void rconv(xtask_task*, const xtask_task_t*);	// Creates aftern too.

void xtask_run(xtask_task* tt, xtask_config cfg) {
#define def(V, D) if(cfg.V == 0) cfg.V = D;
	def(workers, sysconf(_SC_NPROCESSORS_ONLN))
	def(max_leafing, 20)	// These numbers don't acutally matter, current
	def(max_tailing, 40)	// imp doesn't care.
	def(init, NULL)
	def(dest, NULL)

	xtask_setup(cfg.init, cfg.dest, cfg.max_leafing+cfg.max_tailing,
		cfg.workers);
	rconv(tt, &xtask_final);
	xtask_cleanup();
}

// Wrapper to make new-style tasks look like old-style ones.
static int wrapper(void* state, void* data, xtask_aftern_t an) {
#define T ((xtask_task*)data)
	xtask_task* tail = T->func(state, T);
#if VALIDATE
	if(tail && (T->fate & XTASK_FATE_LEAF))
		fprintf(stderr, "WARNING: Task with LEAF fate has tailed!\n");
#endif
	if(tail) {
		// Forests need an extra NULL-task to work
		if(tail->sibling) rconv(tail, &(xtask_task_t){NULL, NULL, an});
		else rpush(tail, an);
		return 0;
	} else return 1;
}

static void rpush(xtask_task* tt, xtask_aftern_t an) {
	for(; tt; tt = tt->sibling) {
		xtask_task_t t = {wrapper, tt, an};
		if(tt->child) rconv(tt->child, &t);
		else xtask_push(&t);
	}
}

static void rconv(xtask_task* tt, const xtask_task_t* parent) {
	int sibs = 0;
	for(xtask_task* stt = tt; stt; stt = stt->sibling) sibs++;
	rpush(tt, xtask_aftern_create(sibs, parent));
}

