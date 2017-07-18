#include "xdata.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct ftask ftask;

struct _xdata_line {
	xdata_line next;
	ftask* writer;
	int numreaders;

	size_t size;
	char data[];
};

struct xdata_state {
	xdata_line head;
};

struct ftask {
	xtask_task task;
	xdata_task func;
	xdata_line out;
	int ni;
	void* in[];
};
static void* ftaskfunc(void*, void*);

typedef struct {
	xtask_task task;
	xdata_line lines;
} ctask;
static void* ctaskfunc(void*, void*);

xdata_line xdata_create(xdata_state* s, size_t size) {
	xdata_line l = malloc(sizeof(struct _xdata_line)+size);
	*l = (struct _xdata_line){ s->head, NULL, 0, size };
	s->head = l;
	return l;
}

void xdata_setdata(xdata_state* s, xdata_line l, void* d) {
	memcpy(l->data, d, l->size);
}

void xdata_prepare(xdata_state* s, xdata_task f, xdata_line o,
	int ni, xdata_line in[]) {

	ftask* t = malloc(sizeof(ftask) + ni*sizeof(void*));
	*t = (ftask){ {ftaskfunc, 0, NULL, NULL}, f, o, ni };
	for(int i=0; i<ni; i++) t->in[i] = in[i];
	o->writer = t;
}

void xdata_run(xdata_task f, xtask_config xc, size_t osize, void* out,
	int ni, void* in[]) {

	xdata_line l = malloc(sizeof(struct _xdata_line) + osize);
	*l = (struct _xdata_line){ NULL, NULL, 0, osize };
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
		xdata_line l = t->in[i];
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
	xdata_state s = { NULL };

	t->out->writer = NULL;
	t->func(xs, &s, t->out, t->in);

	void* tail = t->out->writer;
	if(tail) {
		xtask_task tt = {ctaskfunc, XTASK_FATE_LEAF, NULL, NULL};
		maketree(&s, tail, &tt);
		if(s.head) {
			ctask* c = malloc(sizeof(ctask));
			*c = (ctask){tt, s.head};
			tail = c;
		}
	}
	free(t);
	return tail;
}

static void* ctaskfunc(void* xs, void* vct) {
	ctask* t = vct;
	xdata_line l = t->lines;
	while(l) {
		xdata_line n = l->next;
		free(l);
		l = n;
	}
	free(t);
	return NULL;
}
