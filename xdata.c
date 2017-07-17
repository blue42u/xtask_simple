#include "xdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ftask ftask;

typedef struct {
	size_t offset, size;
	ftask* writer;
	int numreaders;
} line;

struct xdata_state {
	void* out;
	int nlines, lused;
	line* lines;
	size_t size, used;
	void* store;
};

struct ftask {
	xtask_task task;
	xdata_task func;
	size_t outsize;
	union {
		void* d;
		xdata_line l;
	} out;
	int ni;
	union {
		void* d;
		xdata_line l;
	} in[];
};
static void* ftaskfunc(void*, void*);

typedef struct {
	xtask_task task;
	void* store;
} ctask;
static void* ctaskfunc(void*, void*);

xdata_line xdata_create(xdata_state* s, size_t size) {
	if(s->used+size > s->size) {
		while(s->used+size > s->size) s->size *= 2;
		s->store = realloc(s->store, s->size);
	}
	if(s->lused == s->nlines) {
		s->nlines *= 2;
		s->lines = realloc(s->lines, s->nlines*sizeof(line));
	}
	s->lines[s->lused] = (line){ s->used, size, NULL, 0 };
	s->lused += 1;
	s->used += size;
	return s->lused-1;
}

void xdata_setdata(xdata_state* s, xdata_line l, void* d) {
	if(l == 0) memcpy(s->out, d, s->lines[l].size);
	else memcpy(s->store+s->lines[l].offset, d, s->lines[l].size);
}

void xdata_prepare(xdata_state* s, xdata_task f, xdata_line out,
	int ni, xdata_line in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL},
		f, s->lines[out].size, {.l=out}, ni };
	for(int i=0; i<ni; i++) t->in[i].l = in[i];
	s->lines[out].writer = t;
}

void xdata_run(xdata_task f, xtask_config xc, size_t osize, void* out,
	int ni, void* in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL},
		f, osize, {.d=out}, ni };
	for(int i=0; i<ni; i++) t->in[i].d = in[i];
	xtask_run(t, xc);
}

static void maketree(xdata_state* s, ftask* t, xtask_task* p) {
	t->task.sibling = p->child;
	p->child = t;

	xdata_line o = t->out.l;
	if(o == 0) t->out.d = s->out;
	else t->out.d = s->store + s->lines[o].offset;
	for(int i=0; i < t->ni; i++) {
		line* l = &s->lines[t->in[i].l];
		if(++l->numreaders > 1) {
			fprintf(stderr, "Tried to tail with a non-tree graph!\n");
			exit(1);
		}
		t->in[i].d = s->store + l->offset;
		if(l->writer) maketree(s, l->writer, &t->task);
	}
}

static void* ftaskfunc(void* xs, void* vft) {
	ftask* t = vft;
	xdata_state s = { t->out.d,
		1, 1, malloc(sizeof(line)),
		sizeof(int), 0, malloc(sizeof(int)) };
	s.lines[0] = (line){ 0, t->outsize, NULL, 0 };

	void** ins = malloc(t->ni*sizeof(void*));
	for(int i=0; i < t->ni; i++) ins[i] = t->in[i].d;
	t->func(xs, &s, 0, ins);
	free(ins);

	void* tail = NULL;
	if(s.lines[0].writer) {
		ctask* c = malloc(sizeof(ctask));
		*c = (ctask){{ctaskfunc, XTASK_FATE_LEAF, NULL, NULL}, s.store};
		maketree(&s, s.lines[0].writer, &c->task);
		tail = c;
	} else free(s.store);

	free(s.lines);
	free(t);
	return tail;
}

static void* ctaskfunc(void* xs, void* vct) {
	ctask* t = vct;
	free(t->store);
	free(t);
	return NULL;
}
