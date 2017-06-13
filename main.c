#include <stdlib.h>
#include <stdio.h>
#include "xtask_api.h"

#define SAMPLES 200000
#define WORKERS 2

static int* res;

void task(void* dummy, void* vd) {
	int d = *(int*)vd;
	res[d] = d * 2;
}

void print(void* dummy, void* dummy2) {
	printf("After n!\n");
	xtask_push(&xtask_final);
}

int main(void) {
	res = malloc(SAMPLES*sizeof(int));
	int* inds = malloc(SAMPLES*sizeof(int));

	xtask_setup(NULL, NULL, SAMPLES, WORKERS);
	xtask_aftern_t an = xtask_aftern_create(SAMPLES, &xtask_final);
	for(int i=0; i<SAMPLES; i++) {
		inds[i] = i;
		xtask_push(&(xtask_task_t){ task, &inds[i], an });
	}
	xtask_cleanup();

	//for(int i=0; i<SAMPLES; i++)
	//	if(res[i] != i*2) printf("res[%d] = %d\n", i, res[i]);

	free(res);
	free(inds);

	return 0;
}


