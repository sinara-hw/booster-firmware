/*
 * locks.c
 *
 *  Created on: Jun 4, 2018
 *      Author: wizath
 */

#include "config.h"
#include "locks.h"

static SemaphoreHandle_t lock_array[NUM_LOCKS] = { 0 };

void lock_init(void)
{
	for (int i = 0; i < NUM_LOCKS; i++) {
		SemaphoreHandle_t sem = xSemaphoreCreateBinary();
		lock_array[i] = sem;
		xSemaphoreGive(sem);
	}
}

bool lock_take(locks_t sem_num, TickType_t timeout) {
	if (sem_num < NUM_LOCKS) {

		SemaphoreHandle_t semph = lock_array[sem_num];
		if (xSemaphoreTake(semph, timeout)) {
			return true;
		}
	}

	return false;
}

void lock_free(locks_t sem_num) {
	if (sem_num < NUM_LOCKS) {
		SemaphoreHandle_t semph = lock_array[sem_num];
		xSemaphoreGive(semph);
	}
}
