#include "xdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ftask ftask;

typedef struct {
	size_t offset, size, writer;
	int numreaders;
} line;

struct xdata_state {
	ftask* t;
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

static inline size_t suballoc(xdata_state* s, size_t size) {
	if(s->used+size > s->size) {
		while(s->used+size > s->size) s->size *= 2;
		s->store = realloc(s->store, s->size);
	}
	s->used += size;
	return s->used-size;
}

xdata_line xdata_create(xdata_state* s, size_t size) {
	size_t off = suballoc(s, size);
	size_t loff = suballoc(s, sizeof(line));
	*((line*)(s->store+loff)) = (line){ off, size, 0, 0 };
	return loff;
}

void xdata_setdata(xdata_state* s, xdata_line lf, void* d) {
	line* l = s->store + lf;
	if(lf == 0) memcpy(s->t->out.d, d, s->t->outsize);
	else memcpy(s->store + l->offset, d, l->size);
}

void xdata_prepare(xdata_state* s, xdata_task f, xdata_line out,
	int ni, xdata_line in[]) {

	ftask* t;
	size_t toff = suballoc(s, sizeof(ftask) + ni*sizeof(t->in[0]));
	t = s->store + toff;
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, 0, {.l=out}, ni };
	for(int i=0; i<ni; i++) t->in[i].l = in[i];
	((line*)(s->store+out))->writer = toff;
}

void xdata_run(xdata_task f, xtask_config xc, size_t osize, void* out,
	int ni, void* in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, osize, {.d=out}, ni };
	for(int i=0; i<ni; i++) t->in[i].d = in[i];
	xtask_run(t, xc);
	free(t);
}

static void maketree(xdata_state* s, ftask* t, xtask_task* p) {
	t->task.sibling = p->child;
	p->child = t;

	if(t->out.l == 0) {
		t->out.d = s->t->out.d;
		t->outsize = s->t->outsize;
	} else {
		line* o = s->store + t->out.l;
		t->out.d = s->store + o->offset;
		t->outsize = o->size;
	}
	for(int i=0; i < t->ni; i++) {
		line* l = s->store + t->in[i].l;
		if(++l->numreaders > 1) {
			fprintf(stderr, "Tried to tail with a non-tree graph!\n");
			exit(1);
		}
		t->in[i].d = s->store + l->offset;
		if(l->writer > 0) maketree(s, s->store + l->writer, &t->task);
	}
}

static void* ftaskfunc(void* xs, void* vft) {
	ftask* t = vft;
	xdata_state s = { t, sizeof(line), sizeof(line), malloc(sizeof(line)) };
	line* o = s.store;
	*o = (line){0,0,0,0};

	void* ins[t->ni];
	for(int i=0; i < t->ni; i++) ins[i] = t->in[i].d;
	t->func(xs, &s, 0, ins);

	void* tail = NULL;
	o = s.store;
	if(o->writer > 0) {
		size_t coff = suballoc(&s, sizeof(ctask));
		ctask* c = s.store + coff;
		*c = (ctask){{ctaskfunc, XTASK_FATE_LEAF, NULL, NULL}, s.store};
		maketree(&s, s.store + o->writer, &c->task);
		tail = c;
	} else free(s.store);

	return tail;
}

static void* ctaskfunc(void* xs, void* vct) {
	ctask* t = vct;
	free(t->store);
	return NULL;
}
