/*
 * worker.c
 *
 *  Created on: Jun 2, 2017
 *      Author: pnookala
 */

#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include "worker.h"

struct task_desc *execute_task(struct task_desc *task)
{
	sleep(0);

	return 0;
}

void *worker_handler(void *data)
{
	struct mproc_state *mps = (struct mproc_state *) data;
	struct task_desc *task = NULL;

	pthread_barrier_wait(&mps->barrier);

	//while(!(*(mps->kill_master))) {
	for(int count=0; count<(NUM_SAMPLES/WORKERS);count++)
	{
		int i = DEQUEUE();
		execute_task(task);
		ENQUEUE_RESULT(i);
	}

	return NULL;
}


