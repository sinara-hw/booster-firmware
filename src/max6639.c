/*
 * max6639.c
 *
 *  Created on: Nov 21, 2017
 *      Author: wizath
 */
#include "max6639.h"
#include "i2c.h"

bool max6639_test(uint8_t addr)
{
	uint8_t device_id = i2c_read(MAX6639_I2C, addr, MAX6639_REG_DEVID);
	uint8_t man_id = i2c_read(MAX6639_I2C, addr, MAX6639_REG_MANUID);

	printf("[log] max6639 device id %d manufacturer id %d\n", device_id, man_id);

	return (device_id == 0x58 && man_id == 0x4D);
}

void max6639_init(void)
{
	uint8_t address_list[] = {0x2C, 0x2E, 0x2F};
	uint8_t addr = 0x00;

	for (int i = 0; i < 3; i++) {
		addr = address_list[i];

		if (!max6639_test(addr)) {
			printf("[dbg] Device signature failed\n");
			return;
		}

		// device reset
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_GCONFIG, 0b01000000);

		// enable ALERT, THERM, FANFAIL interrupts
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_OUTPUT_MASK, 0b11001111);

		// 50 deg ALERT limit
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(0), 0b00101000);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(1), 0b00101000);

		// 80 deg THERM limit
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_THERM_LIMIT(0), 0b01010000);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(1), 0b01010000);

		// FAN CFG 1
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(0), 0b01110011);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(1), 0b01110011);

		// FAN CFG 2A
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(0), 0b00111011);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(1), 0b00111011);

		// FAN CFG 3A
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(0), 0b01000000);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(1), 0b01000000);

		// FAN MINIMUM TEMP
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_START_TEMP(0), 0b00100011);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_START_TEMP(1), 0b00100011);
	}
}

float max6639_get_temperature(uint8_t addr, uint8_t ch)
{
	float total = 0.0;
	uint8_t temp = i2c_read(MAX6639_I2C, addr, MAX6639_REG_TEMP(ch));
	uint8_t temp_ext = i2c_read(MAX6639_I2C, addr, MAX6639_REG_TEMP_EXT(ch));

	total = (float) temp;
	if (temp_ext & 0x80) total += 0.50;
	if (temp_ext & 0x40) total += 0.25;
	if (temp_ext & 0x20) total += 0.125;

	return total;
}
