/*
 * device.h
 *
 *  Created on: Jan 31, 2019
 *      Author: wizath
 */

#ifndef BOARD_DEVICE_H_
#define BOARD_DEVICE_H_

#include "config.h"
#include "wizchip_conf.h"

typedef struct {
	uint8_t hw_rev;
	double p30_current_sense;

	// network configuration
	uint8_t ipsel;
	uint8_t macsel;

	uint8_t macaddr[6];
	uint8_t ipaddr[4];
	uint8_t gwaddr[4];
	uint8_t netmask[4];

} device_t;

void device_read_revision(void);
device_t * device_get_config(void);
void device_load_network_conf(void);
void device_load_wiznet_conf(wiz_NetInfo * conf);

#endif /* BOARD_DEVICE_H_ */
