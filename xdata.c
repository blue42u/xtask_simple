#include "xdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct xdata_state {
	void* out;
	xdata_line* lines;
	size_t size, used;
	void* store;
};

typedef struct {
	xtask_task task;
	xdata_task func;
	size_t outsize;
	void* out;
	int ni;
	void* in[];
} ftask;
static void* ftaskfunc(void*, void*);

struct xdata_line {
	ssize_t offset;
	size_t size;
	ftask* writer;
	int numreaders;

	xdata_line* next;
};

typedef struct {
	xtask_task task;
	void* store;
} ctask;
static void* ctaskfunc(void*, void*);

xdata_line* xdata_create(xdata_state* s, size_t size) {
	if(s->used+size > s->size) {
		while(s->used+size > s->size) s->size *= 2;
		s->store = realloc(s->store, s->size);
	}
	xdata_line* l = malloc(sizeof(xdata_line));
	*l = (xdata_line){ s->used, size, NULL, 0, s->lines };
	s->lines = l;
	s->used += size;
	return l;
}

void xdata_setdata(xdata_state* s, xdata_line* l, void* d) {
	if(l->offset < 0) memcpy(s->out, d, l->size);
	else memcpy(s->store+l->offset, d, l->size);
}

void xdata_prepare(xdata_state* s, xdata_task f, xdata_line* out,
	int ni, xdata_line* in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, out->size, out, ni };
	for(int i=0; i<ni; i++) t->in[i] = in[i];
	out->writer = t;
}

void xdata_run(xdata_task f, xtask_config xc, size_t osize, void* out,
	int ni, void* in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, osize, out, ni };
	for(int i=0; i<ni; i++) t->in[i] = in[i];
	xtask_run(t, xc);
}

static void maketree(xdata_state* s, ftask* t, xtask_task* p) {
	t->task.sibling = p->child;
	p->child = t;

	xdata_line* o = t->out;
	if(o->offset < 0) t->out = s->out;
	else t->out = s->store + o->offset;
	for(int i=0; i < t->ni; i++) {
		xdata_line* l = t->in[i];
		if(++l->numreaders > 1) {
			fprintf(stderr, "Tried to tail with a non-tree graph!\n");
			exit(1);
		}
		t->in[i] = s->store + l->offset;
		if(l->writer) maketree(s, l->writer, &t->task);
	}
}

static void* ftaskfunc(void* xs, void* vft) {
	ftask* t = vft;
	xdata_line out = {-1, t->outsize, NULL, 0};
	xdata_state s = { t->out, NULL, sizeof(int), 0, malloc(sizeof(int)) };
	t->func(xs, &s, &out, t->in);

	void* tail = NULL;
	if(out.writer) {
		ctask* c = malloc(sizeof(ctask));
		*c = (ctask){{ctaskfunc, XTASK_FATE_LEAF, NULL, NULL}, s.store};
		maketree(&s, out.writer, &c->task);
		tail = c;
	} else free(s.store);

	while(s.lines) {
		xdata_line* n = s.lines->next;
		free(s.lines);
		s.lines = n;
	}
	free(t);
	return tail;
}

static void* ctaskfunc(void* xs, void* vct) {
	ctask* t = vct;
	free(t->store);
	free(t);
	return NULL;
}
