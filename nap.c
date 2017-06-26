#include "xtask.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static xtask_task* nap(void* dummy, xtask_task* data) {
	//sleep(0);
	return NULL;
}

int main(int argc, char** argv) {
	xtask_config xc = {0};
	int samples = 1000;

	char c;
	do {
		c = getopt(argc, argv, "w:s:");
		switch(c) {
		case 'w': xc.workers = atoi(optarg); break;
		case 's': samples = atoi(optarg); break;
		}
	} while(c != -1);

	xtask_task* tasks = malloc(samples*sizeof(xtask_task));
	for(int i=0; i<samples; i++)
		tasks[i] = (xtask_task){nap, XTASK_FATE_LEAF, NULL, &tasks[i+1]};
	tasks[samples-1].sibling = NULL;
	xtask_run(tasks, xc);
	free(tasks);

	return 0;
}


