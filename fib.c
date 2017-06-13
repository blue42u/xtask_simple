#include <stdlib.h>
#include <stdio.h>
#include "xtask_api.h"

typedef struct {
	int n;
	int* out;
	xtask_aftern_t signal;
} fibtask;

typedef struct {
	int* a;
	int* b;
	int* out;
} addtask;

void add(void* dummy, void* task) {
	addtask* t = task;
	*t->out = *t->a + *t->b;
	free(t->a);
	free(t->b);
	free(t);
}

void recurse(void* dummy, void* task) {
	fibtask* t = task;
	if(t->n <= 1) {
		*t->out = t->n;
		if(t->signal) xtask_push(&(xtask_task_t){ NULL, NULL, t->signal });
	} else {
		int* a = malloc(sizeof(int));
		int* b = malloc(sizeof(int));

		addtask* at = malloc(sizeof(addtask));
		*at = (addtask){ a, b, t->out };

		xtask_aftern_t an = xtask_aftern_create(2,
			&(xtask_task_t){ add, at, t->signal });

		fibtask* ft1 = malloc(sizeof(fibtask));
		*ft1 = (fibtask){ t->n-1, a, an };

		fibtask* ft2 = malloc(sizeof(fibtask));
		*ft2 = (fibtask){ t->n-2, b, an };

		xtask_push(&(xtask_task_t){ recurse, ft1, NULL });
		xtask_push(&(xtask_task_t){ recurse, ft2, NULL });
	}
	free(t);
}

#define FIB_INDEX 20
#define WORKERS 2

int main(void) {
	xtask_setup(NULL, NULL, 10000, WORKERS);

	xtask_aftern_t finish = xtask_aftern_create(1, &xtask_final);

	int out;
	fibtask* t = malloc(sizeof(fibtask));
	*t = (fibtask){ FIB_INDEX, &out, finish };
	xtask_push(&(xtask_task_t){ recurse, t, NULL });

	xtask_cleanup();

	//printf("Out: %d\n", out);

	return 0;
}


