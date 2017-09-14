/*
 * ads7924.h
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#ifndef ADS7924_H_
#define ADS7924_H_

#include "config.h"

#define ADS7924_ADDRESS 	0x49
#define ADS7924_I2C			I2C1

#define MODECNTRL_REG		0x00
#define INTCNTRL			0x01
#define DATA0_REG			0x02
#define DATA1_REG			0x04
#define DATA2_REG			0x06
#define DATA3_REG			0x08
#define UT0_REG				0x0A
#define LT0_REG				0x0B
#define UT1_REG				0x0C
#define LT1_REG				0x0D
#define UT2_REG				0x0E
#define LT2_REG				0x0F
#define UT3_REG				0x10
#define LT3_REG				0x11
#define INTCONFIG_REG		0x12
#define SLPCONFIG_REG		0x13
#define ACQCONFIG_REG		0x14
#define PWRCONFIG_REG		0x15
#define RESET_REG			0x16

#define RESET_CMD			0b10101010

static uint8_t data_channels[] = {DATA0_REG, DATA1_REG, DATA2_REG, DATA3_REG};
static uint8_t upper_thresholds[] = {DATA0_REG, DATA1_REG, DATA2_REG, DATA3_REG};
static uint8_t lower_thresholds[] = {DATA0_REG, DATA1_REG, DATA2_REG, DATA3_REG};

void ads7924_reset(void)
{
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, RESET_REG, RESET_CMD);
}

void ads7924_start(void)
{

}

uint16_t ads7924_get_channel_data(uint8_t ch)
{
	if (ch > 3) return 0;
	uint8_t base_addr = data_channels[ch];

	uint8_t msb = i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr);
	uint8_t lsb = i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr + 1);

	return (msb << 8) | lsb;
}




#endif /* ADS7924_H_ */
