/*
 * ad5627.c
 *
 *  Created on: Jul 13, 2018
 *      Author: wizath
 */

#ifndef DEVICES_AD5627_C_
#define DEVICES_AD5627_C_

#include "config.h"
#include "i2c.h"

void ad5627_init(void)
{
	// enable internal reference
	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 127);
	i2c_write_byte(I2C1, 0);
	i2c_write_byte(I2C1, 1);
	i2c_stop(I2C1);
}

void ad5627_set_value(uint8_t ch, uint16_t value)
{
	uint16_t regval = value & 0xFFF;

	if (ch == 0) {
		i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
		i2c_write_byte(I2C1, 0b01011000);

		i2c_write_byte(I2C1, (regval & 0xFF0) >> 4);
		i2c_write_byte(I2C1, (regval & 0x00F) << 4);

		i2c_stop(I2C1);
	}

	if (ch == 1) {
		i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
		i2c_write_byte(I2C1, 0b01011001);

		i2c_write_byte(I2C1, (regval & 0xFF0) >> 4);
		i2c_write_byte(I2C1, (regval & 0x00F) << 4);

		i2c_stop(I2C1);
	}

	if (ch == 3) {
		i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
		i2c_write_byte(I2C1, 0b01011111);

		i2c_write_byte(I2C1, (regval & 0xFF0) >> 4);
		i2c_write_byte(I2C1, (regval & 0x00F) << 4);

		i2c_stop(I2C1);
	}
}

void ad5627_set_voltage(float v1, float v2)
{
	// ref voltage value adjusted slightly
	uint16_t value1 = (uint16_t) ((float) ((v1 * 4095.0f) / ( 2 * 2.2f )));
	uint16_t value2 = (uint16_t) ((float) ((v2 * 4095.0f) / ( 2 * 2.2f )));

	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 0b01011000);
	i2c_write_byte(I2C1, (value1 & 0xFF0) >> 4);
	i2c_write_byte(I2C1, (value1 & 0x00F) << 4);
	i2c_stop(I2C1);

	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 0b01011001);
	i2c_write_byte(I2C1, (value2 & 0xFF0) >> 4);
	i2c_write_byte(I2C1, (value2 & 0x00F) << 4);
	i2c_stop(I2C1);
}

#endif /* DEVICES_AD5627_C_ */
