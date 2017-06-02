/*
 * xtask_api.h
 *
 *  Created on: Jun 2, 2017
 *      Author: pnookala
 */

#ifndef XTASK_API_H_
#define XTASK_API_H_

#include <pthread.h>

void xtask_setup(int queue_size, int workers);
void xtask_cleanup();

void xtask_push(int type, int threads, int id, void *params);
void xtask_poll(int *id, void **params);

#endif /* __GEMTC_API_H */


#endif /* XTASK_API_H_ */
