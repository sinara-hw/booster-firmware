/*
 * device.h
 *
 *  Created on: Jan 31, 2019
 *      Author: wizath
 */

#ifndef BOARD_DEVICE_H_
#define BOARD_DEVICE_H_

#include "config.h"

typedef struct {
	uint8_t hw_rev;
	double p30_current_sense;
	double sw_ovc_current_value;
} device_t;

void device_read_revision(void);
device_t * device_get_config(void);

#endif /* BOARD_DEVICE_H_ */
