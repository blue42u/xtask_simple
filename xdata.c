#include "xdata.h"
#include <stdlib.h>

// Different structures for things
typedef struct {
	xtask_task task;
	xdata_task func;
	int frommain, ni, no;
	void* data[];
} ftask;

typedef struct {
	ftask* writer;
	void* mem;
	ftask* reader;
} dataent;

struct xdata_state {
	int size, used;
	dataent* conn;
};

static inline dataent* getent(xdata_state* s, void* m) {
	for(int i=0; i < s->used; i++)
		if(s->conn[i].mem == m) return &s->conn[i];
	return NULL;
}

static inline dataent* newent(xdata_state* s, void* m) {
	if(s->size == s->used) {
		s->size *= 2;
		s->conn = realloc(s->conn, s->size*sizeof(dataent));
	}
	return &s->conn[s->used++];
}

void* xdata_create(xdata_state* s, size_t size) {
	void* m = malloc(size);
	*newent(s, m) = (dataent){NULL, m, NULL};
	return m;
}

static void* taskfunc(void* xstate, void* vt) {
	ftask* t = vt;
	// Setup the state for this task
	xdata_state s = {t->ni+t->no+10, t->ni+t->no};
	s.conn = malloc(s.size*sizeof(dataent));
	for(int i=0; i < t->ni+t->no; i++)
		s.conn[i] = (dataent){NULL, t->data[i], NULL};

	// Run the task
	t->func(xstate, &s, t->data, &t->data[t->ni]);

	// Clean up unused inputs, if not app-managed memory
	if(!t->frommain)
		for(int i=0; i < t->ni; i++)
			if(s.conn[i].reader == NULL) free(s.conn[i].mem);

	// Gather together the new tasks
	ftask* tail = NULL;
	for(int i=t->ni; i < t->ni+t->no; i++) {
		if(s.conn[i].writer) {
			s.conn[i].writer->task.sibling = tail;
			tail = s.conn[i].writer;
		}
	}

	// Free the task and state, and tail with the new tasks
	free(t);
	free(s.conn);
	return tail;
}

void xdata_prepare(xdata_state* s, xdata_task f, int ni, void* in[],
	int no, void* ot[]) {

	ftask* t = malloc(sizeof(ftask)+(ni+no)*sizeof(void*));
	*t = (ftask){{taskfunc, 0, NULL, NULL}, f, 0, ni, no};
	for(int i=0; i<no; i++) {
		getent(s, ot[i])->writer = t;
		t->data[ni+i] = ot[i];
	}
	for(int i=0; i<ni; i++) {
		t->data[i] = in[i];
		dataent* de = getent(s, in[i]);
		de->reader = t;
		if(de->writer) {
			de->writer->task.sibling = t->task.child;
			t->task.child = de->writer;
		}
	}
}

void xdata_run(xdata_task f, xtask_config xc, int ni, void* in[],
	int no, void* ot[]) {

	ftask* t = malloc(sizeof(ftask)+(ni+no)*sizeof(void*));
	*t = (ftask){{taskfunc, 0, NULL, NULL}, f, 1, ni, no};
	for(int i=0; i<ni; i++) t->data[i] = in[i];
	for(int i=0; i<no; i++) t->data[ni+i] = ot[i];
	xtask_run(t, xc);
}
