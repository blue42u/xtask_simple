#include <stdlib.h>
#include <stdio.h>
#include "xtask_api.h"

typedef struct {
	int n;
	int* out;
} fibtask;

typedef struct {
	int* data;
	int* out;
} addtask;

int add(void* dummy, void* task, xtask_aftern_t tail) {
	addtask* t = task;
	*t->out = t->data[0] + t->data[1];
	free(t->data);
	free(t);
	return 1;	// Adds do things, so don't tail.
}

int recurse(void* dummy, void* task, xtask_aftern_t tail) {
	fibtask* t = task;
	if(t->n <= 1) {
		*t->out = t->n;
		free(t);
		return 1;	// All done here, so don't tail
	} else {
		int* ab = malloc(2*sizeof(int));

		// First create the after-n
		addtask* at = malloc(sizeof(addtask));
		*at = (addtask){ ab, t->out };
		xtask_aftern_t an = xtask_aftern_create(2,
			&(xtask_task_t){ add, at, tail });

		fibtask* ft1 = malloc(sizeof(fibtask));
		*ft1 = (fibtask){ t->n-1, &ab[0] };
		xtask_push(&(xtask_task_t){ recurse, ft1, an });

		fibtask* ft2 = malloc(sizeof(fibtask));
		*ft2 = (fibtask){ t->n-2, &ab[1] };
		xtask_push(&(xtask_task_t){ recurse, ft2, an });

		free(t);
		return 0;	// Tail, we used the + as our replacement
	}
}

#define FIB_INDEX 30
#define WORKERS 6

int main(void) {
	xtask_setup(NULL, NULL, 10000, WORKERS);

	xtask_aftern_t finish = xtask_aftern_create(1, &xtask_final);

	int out;
	fibtask* t = malloc(sizeof(fibtask));
	*t = (fibtask){ FIB_INDEX, &out };
	xtask_push(&(xtask_task_t){ recurse, t, finish });

	xtask_cleanup();

	//printf("Out: %d\n", out);

	return 0;
}


