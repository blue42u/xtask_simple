/*
 * main.c
 *
 *  Created on: May 31, 2017
 *      Author: pnookala
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sched.h>
#include <time.h>
#include "worker.h"
#include "basicqueue.h"
#include "xtask_api.h"

int main(void)
{
	int *id = (int *) malloc(sizeof(int));
	void *params;

	*id = -1;

	xtask_setup(QUEUE_SIZE, WORKERS);

	for(int i=0; i<NUM_SAMPLES; i++)
	{
		xtask_push(1,1, i+1, NULL);
	}

	for(int i=0; i<NUM_SAMPLES; i++)
	{
		xtask_poll(id,&params);
#ifdef VERBOSE
		printf("Result : %d \n", *id);
#endif
	}

	xtask_cleanup();

	//while(1);
	free(id);

	return(0);
}


