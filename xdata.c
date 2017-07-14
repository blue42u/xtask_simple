#include "xdata.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct {
	xtask_task t;
	xdata_task f;
	int numin, numout;
	void* data[];
} task;

typedef struct {
	void* mem;
	task* task;
} dataent;

// One Task's state
struct xdata_state {
	int size, next;
	dataent* writers;
};

typedef struct {
	xtask_task t;
	xdata_state s;
	int start;
	int real;
} cleantask;

static inline void* aa(void* x) {
	if(x == NULL) {
		fprintf(stderr, "Out of memory!\n");
		exit(EXIT_FAILURE);
	}
	return x;
}

void* xdata_create(xdata_state* s, size_t size) {
	if(s->size == s->next) {
		s->size *= 2;
		s->writers = aa(realloc(s->writers, s->size*sizeof(dataent)));
	}
	void* m = aa(malloc(size));
	s->writers[s->next++] = (dataent){ m, NULL };
	printf("New var %p\n", m);
	return m;
}

static void addsubtree(xdata_state* s, task** tt, task* t) {
	if(t) {
		printf("Subtask %p\n", t);
		// If the task is already on this layer, don't bother.
		for(task* s = *tt; s; s = s->t.sibling)
			if(s == t) return;

		// Otherwise, add it in, and build the subtree
		t->t.sibling = *tt;
		*tt = t;
		for(int i=0; i < t->numin; i++) for(int j=0; j < s->next; j++)
			if(s->writers[j].mem == t->data[i]) {
				printf("%p(%p) -> %p\n", t,
					s->writers[j].mem, s->writers[j].task);
				addsubtree(s, &t->t.child, s->writers[j].task);
			}
	}
}

static void* cleanup(void* dummy, void* vct) {
	cleantask* ct = vct;
	if(ct->real) printf("In cleantask %p\n", ct);
	for(int i=ct->start; i < ct->s.next; i++)
		free(ct->s.writers[i].mem);
	free(ct->s.writers);
	if(ct->real) free(ct);
	return NULL;
}

static void* taskfunc(void* xstate, void* vtask) {
	task* t = vtask;
	printf("Running task %p\n", t);

	// Setup the state, to remember things
	xdata_state s = { t->numout+10, t->numout,
		malloc((t->numout+10)*sizeof(dataent)) };
	for(int i=0; i < t->numout; i++)
		s.writers[i] = (dataent){ t->data[t->numin+i], NULL };

	// Do the task
	t->f(xstate, &s, t->data, &t->data[t->numin]);

	// Figure out how the outputs get written. Currently assumes tree-ish.
	task* res = NULL;
	for(int i=0; i < t->numout; i++) {
		printf("%p(%p) -> %p\n", t, s.writers[i].mem,
			s.writers[i].task);
		addsubtree(&s, &res, s.writers[i].task);
	}
	free(t);

	if(s.next > t->numout) {
		cleantask* ct = aa(malloc(sizeof(cleantask)));
		printf("Spawning cleantask %p\n", ct);
		*ct = (cleantask){{cleanup, XTASK_FATE_LEAF, res, NULL}, s,
			t->numout-1, 1};
		return ct;
	} else {
		cleanup(NULL, &(cleantask){{}, s, t->numout-1, 0});
		return res;
	}
}

static inline task* maketask(xdata_task f, int ni, void* i[], int no, void* o[]) {
	task* t = malloc(sizeof(task)+(ni+no)*sizeof(void*));
	*t = (task){ {taskfunc, 0, NULL, NULL}, f, ni, no };
	for(int j=0; j<ni; j++) t->data[j] = i[j];
	for(int j=0; j<no; j++) t->data[ni+j] = o[j];
	return t;
}

void xdata_prepare(xdata_state* s, xdata_task f, int nin, void* ins[], int nout,
	void* outs[]) {
	task* t = maketask(f, nin, ins, nout, outs);
	printf("Prepping task %p\n", t);
	for(int i=0; i<nout; i++) for(int j=0; j < s->next; j++)
		if(s->writers[j].mem == outs[i])
			s->writers[j].task = t;
}

void xdata_run(xdata_task f, xtask_config xc, int nin, void* ins[],
	int nout, void* outs[]) {
	task* t = maketask(f, nin, ins, nout, outs);
	printf("First task: %p\n", t);
	xtask_run(t, xc);
}
