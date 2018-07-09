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

		i2c_write(MAX6639_I2C, addr, MAX6639_REG_GCONFIG, 0b00110000);

		// PWM MODE
		// RATE OF CHANGE 0.5s
		// TEMP CHAN OFF (PWM MODE)
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(0), 0b10110010);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(1), 0b10110010);


		// Step size default
		// Temp step size default
		// PWM Output 100% = 0 duty cycle
		// Minimum speed off
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(0), 0b00000000);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(1), 0b00000000);


		// SPIN UP Disable
		// THERM Limit to full speed
		// Enable pulse stretching
		// PWM Output frequency 20 Hz
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(0), 0b11100010);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(1), 0b11100010);

		// 10% duty cycle
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGTDUTY(0), 0);
		i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGTDUTY(1), 0);

//		// global conf register 0x4
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_GCONFIG, 0b10110000);
//
//		// fan conf 1 0x10
//		// PWM MODE OFF
//		// RATE OF CHANGE 1s
//		// RPM SPEED 4000
//		// channel 2 drives auto
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(0), 0b01011101);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG1(1), 0b01011101);
//
//		// fan conf 2a
//		// minimum speed
//		// rpm step size 0011
//		// temp step size 01
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(0), 0b00111100);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG2a(1), 0b00111100);
//
//		// fan conf 2b defaults
//
//		// fan conf 3b
//		// spin up disable
//		// therm to full speed disable
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(0), 0b10100001);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_CONFIG3(1), 0b10100001);
//
//		// fan minimum speed
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGET_CNT(0), 0b01101100);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGET_CNT(1), 0b01101100);
//
//		// fan minimum start temp
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_START_TEMP(0), 10);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_FAN_START_TEMP(1), 10);
//
////		// enable ALERT, THERM, FANFAIL interrupts
////		i2c_write(MAX6639_I2C, addr, MAX6639_REG_OUTPUT_MASK, 0b11001111);
//
//		// 60 deg ALERT limit
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(0), 0b00110010);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(1), 0b00110010);
//
//		// 80 deg THERM limit
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_THERM_LIMIT(0), 0b01010000);
//		i2c_write(MAX6639_I2C, addr, MAX6639_REG_ALERT_LIMIT(1), 0b01010000);
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
