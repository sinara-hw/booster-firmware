/*
 * dac7571.c
 *
 *  Created on: Jul 13, 2018
 *      Author: wizath
 */

#include "config.h"
#include "i2c.h"

void dac7571_set_value(uint16_t value)
{
	uint8_t first_byte = (value & 0xFF00) >> 8;
	uint8_t second_byte = value & 0xFF;

	i2c_start(I2C1, I2C_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, first_byte);
	i2c_write_byte(I2C1, second_byte);
	i2c_stop(I2C1);
}

void dac7571_set_voltage(float value)
{
	uint16_t dacval = (uint16_t) ((float) ((value * 4095.0f) / ( 3.3f )));
	uint8_t first_byte = (dacval & 0xFF00) >> 8;
	uint8_t second_byte = dacval & 0xFF;

	i2c_start(I2C1, I2C_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, first_byte);
	i2c_write_byte(I2C1, second_byte);
	i2c_stop(I2C1);
}
