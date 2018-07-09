/*
 * locks.h
 *
 *  Created on: Jun 6, 2018
 *      Author: wizath
 */

#ifndef LOCKS_H_
#define LOCKS_H_

typedef enum {
	I2C_LOCK,
	SPI_LOCK,
	ETH_LOCK,

	NUM_LOCKS
} locks_t;

void lock_init(void);
bool lock_take(locks_t sem_num, TickType_t timeout);
void lock_free(locks_t sem_num);

#endif /* LOCKS_H_ */
