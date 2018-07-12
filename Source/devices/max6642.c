/*
 * max6642.c
 *
 *  Created on: Jul 12, 2018
 *      Author: wizath
 */

#include "i2c.h"

#define TEMP_SENS_I2C_ADDR			0x4A
#define TEMP_SENS_LOCAL_TEMP_L		0x10
#define TEMP_SENS_LOCAL_TEMP_H		0x00
#define TEMP_SENS_REMOTE_TEMP_L		0x11
#define TEMP_SENS_REMOTE_TEMP_H		0x01

void max6642_init(void)
{
	i2c_write(I2C1, I2C_MODULE_TEMP, 0x09, 128);
}

float max6642_get_local_temp(void)
{
	float total = 0.0;
	uint8_t temp = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_LOCAL_TEMP_H);
	uint8_t temp_ext = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_LOCAL_TEMP_L);

	total = (float) temp;
	if (temp_ext & 0x40) total += 0.250;
	if (temp_ext & 0x80) total += 0.500;
	if (temp_ext & 0xC0) total += 0.750;

	return total;
}

float max6642_get_remote_temp(void)
{
	float total = 0.0;
	uint8_t temp = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_REMOTE_TEMP_H);
	uint8_t temp_ext = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_REMOTE_TEMP_L);

	total = (float) temp;
	if (temp_ext & 0x40) total += 0.250;
	if (temp_ext & 0x80) total += 0.500;
	if (temp_ext & 0xC0) total += 0.750;

	return total;
}
