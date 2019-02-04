/*
 * ads7924.c
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#include "i2c.h"
#include "ads7924.h"

static uint8_t data_channels[] = {DATA0_REG, DATA1_REG, DATA2_REG, DATA3_REG};
static uint8_t upper_thresholds[] = {UT0_REG, UT1_REG, UT2_REG, UT3_REG};
static uint8_t lower_thresholds[] = {LT0_REG, LT1_REG, LT2_REG, LT3_REG};
static uint8_t modes[] = {0b000000, 0b100000, 0b110000, 0b110010, 0b110001, 0b110011, 0b111001, 0b111011, 0b111111};

void ads7924_reset(void)
{
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, RESET_REG, RESET_CMD);
}

void ads7924_set_threshholds(uint8_t ch, uint8_t upper, uint8_t lower)
{
	if (ch > 3) return;
	uint8_t u_addr = upper_thresholds[ch];
	uint8_t l_addr = lower_thresholds[ch];

	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, u_addr, upper);
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, l_addr, lower);
}

void ads7924_get_threshholds(uint8_t ch, uint8_t * upper, uint8_t * lower)
{
	if (ch > 3) return;
	uint8_t u_addr = upper_thresholds[ch];
	uint8_t l_addr = lower_thresholds[ch];

	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, u_addr, upper);
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, l_addr, lower);
}

void ads7924_set_mode(uint8_t mode)
{
	if (mode > 8) return;

	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, MODECNTRL_REG, modes[mode] << 2);
}

void ads7924_set_alarm(uint8_t ch)
{
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, INTCNTRL_REG, ch);
}

uint8_t ads7924_get_alarm(void)
{
	// returns  XXXX  -  XXXX
	// 		   status - enable
	uint8_t alarm;
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, INTCNTRL_REG, &alarm);
	return alarm;
}

uint16_t ads7924_get_channel_data(uint8_t ch)
{
	if (ch > 3) return 0;
	uint8_t base_addr = data_channels[ch];

	uint8_t msb, lsb = 0;
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr, &msb);
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr + 1, &lsb);

	return ((msb << 8) | lsb) >> 4;
}

double ads7924_get_channel_voltage(uint8_t ch)
{
	if (ch > 3) return 0;
	uint8_t base_addr = data_channels[ch];

	uint8_t msb, lsb = 0;
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr, &msb);
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr + 1, &lsb);

	uint16_t raw = ((msb << 8) | lsb) >> 4;
	return (double) ((raw * 2.50f) / 4095);
}

void ads7924_get_data(uint16_t * data)
{
	uint8_t lb, ub;

	i2c_start(ADS7924_I2C, ADS7924_ADDRESS, I2C_Direction_Transmitter, 0);
	i2c_write_byte(ADS7924_I2C, DATA0_REG | 0x80); // multi read
	i2c_stop(ADS7924_I2C);

	i2c_start(ADS7924_I2C, ADS7924_ADDRESS, I2C_Direction_Receiver, 0);
	for (int i = 0; i < 8; i++)
	{
		i2c_read_byte_ack(ADS7924_I2C, &ub);
		i == 7 ? i2c_read_byte_nack(ADS7924_I2C, &lb) : i2c_read_byte_ack(ADS7924_I2C, &lb);
		*data++ = ((ub << 8) | lb) >> 4;
	}

	i2c_stop(ADS7924_I2C);
}

void ads7924_init(void)
{
	uint8_t id = 0;
	i2c_read(I2C1, ADS7924_ADDRESS, 0x16, &id);

	ads7924_reset();
	for (int i = 0; i < 84000; i++) {}; // wait for power-up sequence to end

	ads7924_set_mode(MODE_AUTO_SCAN_SLEEP);
	i2c_write(I2C1, ADS7924_ADDRESS, 0x12, 0b11100010);

	// sleep to 10ms per measurement
//	i2c_write(I2C1, ADS7924_ADDRESS, 0x13, 0b00000010);
//	i2c_write(I2C1, ADS7924_ADDRESS, 0x14, 0b00011111);

	// set alert for i30 at 600mA
	// 1 bit = AVDD/256
	// upper limit 600mA
	ads7924_set_threshholds(0, 160, 0);
}

void ads7924_enable_alert(void)
{
	// enable alarm for channel 0 (30V current)
	i2c_write(I2C1, ADS7924_ADDRESS, 0x01, 0b00000001);
}

void ads7924_disable_alert(void)
{
	i2c_write(I2C1, ADS7924_ADDRESS, 0x01, 0b00000000);
}

uint8_t ads7924_clear_alert(void)
{
	uint8_t alert = 0;
	i2c_read(I2C1, ADS7924_ADDRESS, 0x01, &alert);
	return alert;
}

void ads7924_stop(void)
{
	ads7924_set_mode(MODE_IDLE);
}
