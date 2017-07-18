#include "xdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ftask ftask;

typedef struct line {
	struct line* next;
	ftask* writer;
	int numreaders;

	size_t size;
	char data[];
} line;

struct xdata_state {
	int nlines;
	line* head;
	line* tail;
};

struct ftask {
	xtask_task task;
	xdata_task func;
	line* out;
	int ni;
	void* in[];
};
static void* ftaskfunc(void*, void*);

typedef struct {
	xtask_task task;
	line* lines;
} ctask;
static void* ctaskfunc(void*, void*);

xdata_line xdata_create(xdata_state* s, size_t size) {
	line* l = malloc(sizeof(line)+size);
	*l = (line){ NULL, NULL, 0, size };
	s->tail->next = l;
	s->tail = l;
	printf("Creating %p\n", l);
	return s->nlines++;
}

static inline line* getlin(xdata_state* s, xdata_line li) {
	line* l = s->head;
	for(int i=0; i<li; i++) l = l->next;
	return l;
}

void xdata_setdata(xdata_state* s, xdata_line li, void* d) {
	line* l = getlin(s, li);
	memcpy(l->data, d, l->size);
}

void xdata_prepare(xdata_state* s, xdata_task f, xdata_line out,
	int ni, xdata_line in[]) {

	line* o = getlin(s, out);
	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, o, ni };
	for(int i=0; i<ni; i++) t->in[i] = getlin(s, in[i]);
	o->writer = t;
}

void xdata_run(xdata_task f, xtask_config xc, size_t osize, void* out,
	int ni, void* in[]) {

	line* l = malloc(sizeof(line) + osize);
	*l = (line){ NULL, NULL, 0, osize };
	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, l, ni };
	for(int i=0; i<ni; i++) t->in[i] = in[i];
	xtask_run(t, xc);
	memcpy(out, &l->data, osize);
	free(l);
}


static void maketree(xdata_state* s, ftask* t, xtask_task* p) {
	t->task.sibling = p->child;
	p->child = t;

	for(int i=0; i < t->ni; i++) {
		line* l = t->in[i];
		if(++(l->numreaders) > 1) {
			fprintf(stderr, "Tried to tail with a non-tree graph!\n");
			exit(1);
		}
		t->in[i] = &l->data;
		if(l->writer) maketree(s, l->writer, &t->task);
	}
}

static void* ftaskfunc(void* xs, void* vft) {
	ftask* t = vft;
	xdata_state s = { 1, t->out, t->out };
	t->out->writer = NULL;
	t->out->next = NULL;

	t->func(xs, &s, 0, t->in);

	void* tail = t->out->writer;
	if(tail) {
		xtask_task tt = {ctaskfunc, XTASK_FATE_LEAF, NULL, NULL};
		maketree(&s, tail, &tt);
		if(s.head->next) {
			ctask* c = malloc(sizeof(ctask));
			*c = (ctask){tt, s.head->next};
			tail = c;
		}
	}
	free(t);
	return tail;
}

static void* ctaskfunc(void* xs, void* vct) {
	ctask* t = vct;
	line* l = t->lines;
	while(l) {
		line* n = l->next;
		printf("Cleaning up %p\n", l);
		free(l);
		l = n;
	}
	free(t);
	return NULL;
}
