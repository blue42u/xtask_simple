#include "xtask.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
	xtask_task task;
	int n;
	int* out;
} fibdata;

typedef struct {
	xtask_task task;
	fibdata a;
	fibdata b;
	int aout, bout;
	int* out;
} fulldata;

#define add ((xtask_func)_add)
static void* _add(void* dummy, fulldata* data) {
	*data->out = data->aout + data->bout;
	free(data);
	return NULL;
}

#define fib ((xtask_func)_fib)
static fulldata* _fib(void* dummy, fibdata* data) {
	if(data->n <= 1) {
		*data->out = data->n;
		return NULL;
	} else {
		fulldata* fd = malloc(sizeof(fulldata));
		*fd = (fulldata){
			{add, XTASK_FATE_LEAF, &fd->a, NULL},
			{{fib, 0, NULL, &fd->b}, data->n-1, &fd->aout},
			{{fib, 0, NULL, NULL},   data->n-2, &fd->bout},
			0, 0, data->out,
		};
		return fd;
	}
}

int main(int argc, char** argv) {
	xtask_config xc = {0};
	int fibindex = 20;

	char c;
	do {
		c = getopt(argc, argv, "w:f:");
		switch(c) {
		case 'w': xc.workers = atoi(optarg); break;
		case 'f': fibindex = atoi(optarg); break;
		}
	} while(c != -1);

	int out;
	fibdata fd = {{fib, 0, NULL, NULL}, fibindex, &out};
	xtask_run(&fd, xc);
	printf("%d\n", out);
}
