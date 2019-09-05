/*
 * mcp3221.h
 *
 *  Created on: Sep 4, 2019
 *      Author: wizath
 */

#ifndef DEVICES_MCP3221_H_
#define DEVICES_MCP3221_H_

#include "config.h"

#define MCP3221_ADDR	0x4D

uint16_t mcp3221_get_data(void);
double mcp3221_get_voltage(void);

#endif /* DEVICES_MCP3221_H_ */
