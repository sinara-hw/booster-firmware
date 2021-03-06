/*
 * ads7924.c
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#include "i2c.h"
#include "ads7924.h"
#include "device.h"

static uint8_t data_channels[] = {DATA0_REG, DATA1_REG, DATA2_REG, DATA3_REG};
static uint8_t upper_thresholds[] = {UT0_REG, UT1_REG, UT2_REG, UT3_REG};
static uint8_t lower_thresholds[] = {LT0_REG, LT1_REG, LT2_REG, LT3_REG};
static uint8_t modes[] = {0b000000, 0b100000, 0b110000, 0b110010, 0b110001, 0b110011, 0b111001, 0b111011, 0b111111};

void ads7924_reset(void)
{
	vPortEnterCritical();
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, RESET_REG, RESET_CMD);
	vPortExitCritical();
}

void ads7924_set_threshholds(uint8_t ch, uint8_t upper, uint8_t lower)
{
	if (ch > 3) return;
	uint8_t u_addr = upper_thresholds[ch];
	uint8_t l_addr = lower_thresholds[ch];

	vPortEnterCritical();

	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, u_addr, upper);
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, l_addr, lower);

	vPortExitCritical();
}

void ads7924_get_threshholds(uint8_t ch, uint8_t * upper, uint8_t * lower)
{
	if (ch > 3) return;
	uint8_t u_addr = upper_thresholds[ch];
	uint8_t l_addr = lower_thresholds[ch];

	vPortEnterCritical();

	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, u_addr, upper);
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, l_addr, lower);

	vPortExitCritical();
}

void ads7924_set_mode(uint8_t mode)
{
	if (mode > 8) return;

	vPortEnterCritical();

	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, MODECNTRL_REG, modes[mode] << 2);

	vPortExitCritical();
}

void ads7924_set_alarm(uint8_t ch)
{

	vPortEnterCritical();
	i2c_write(ADS7924_I2C, ADS7924_ADDRESS, INTCNTRL_REG, ch);
	vPortExitCritical();
}

uint8_t ads7924_get_alarm(void)
{
	// returns  XXXX  -  XXXX
	// 		   status - enable
	uint8_t alarm;
	vPortEnterCritical();
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, INTCNTRL_REG, &alarm);
	vPortExitCritical();
	return alarm;
}

uint16_t ads7924_get_channel_data(uint8_t ch)
{
	if (ch > 3) return 0;
	uint8_t base_addr = data_channels[ch];

	uint8_t msb, lsb = 0;

	vPortEnterCritical();
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr, &msb);
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr + 1, &lsb);
	vPortExitCritical();

	return ((msb << 8) | lsb) >> 4;
}

double ads7924_get_channel_voltage(uint8_t ch)
{
	if (ch > 3) return 0;
	uint8_t base_addr = data_channels[ch];

	uint8_t msb, lsb = 0;

	vPortEnterCritical();
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr, &msb);
	vPortExitCritical();

	vTaskDelay(2);

	vPortEnterCritical();
	i2c_read(ADS7924_I2C, ADS7924_ADDRESS, base_addr + 1, &lsb);
	vPortExitCritical();

	vTaskDelay(2);

	device_t * dev = device_get_config();

	uint16_t raw = ((msb << 8) | lsb) >> 4;
	// return appropriate calculation due to change of AVDD to 3.3V in hw rev 4
	if (dev->hw_rev < 4)
		return (double) ((raw * 2.50f) / 4095);
	else
		return (double) ((raw * 3.30f) / 4095);
}

void ads7924_get_data(uint16_t * data)
{
	uint8_t lb, ub;

	vPortEnterCritical();
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
	vPortExitCritical();

}

void ads7924_init(void)
{
	uint8_t id = 0;

	vPortEnterCritical();

	i2c_read(I2C1, ADS7924_ADDRESS, 0x16, &id);

	ads7924_reset();
	for (int i = 0; i < 84000; i++) {}; // wait for power-up sequence to end


	i2c_write(I2C1, ADS7924_ADDRESS, 0x12, 0b00000010);//0b11100010);

	// sleep to 10ms per measurement
	i2c_write(I2C1, ADS7924_ADDRESS, 0x13, 0b00000000);//i2c_write(I2C1, ADS7924_ADDRESS, 0x13, 0b00000010);
	i2c_write(I2C1, ADS7924_ADDRESS, 0x14, 0b00011111);

	//ads7924_set_mode(MODE_AUTO_SCAN_SLEEP);

	//i2c_read(I2C1, ADS7924_ADDRESS, 0x01, &id);


	ads7924_set_mode(MODE_MANUAL_SCAN);

	vPortExitCritical();
}

void ads7924_enable_alert(void)
{
	// enable alarm for channel 0 (30V current)
	vPortEnterCritical();
	i2c_write(I2C1, ADS7924_ADDRESS, 0x01, 0b00000001);
	vPortExitCritical();
}

void ads7924_disable_alert(void)
{
	vPortEnterCritical();
	i2c_write(I2C1, ADS7924_ADDRESS, 0x01, 0b00000000);
	vPortExitCritical();
}

uint8_t ads7924_clear_alert(void)
{
	uint8_t alert = 0;
	vPortEnterCritical();
	i2c_read(I2C1, ADS7924_ADDRESS, 0x01, &alert);
	vPortExitCritical();
	return alert;
}

void ads7924_stop(void)
{
	vPortEnterCritical();
	ads7924_set_mode(MODE_IDLE);
	vPortExitCritical();
}

uint8_t ads7924_getCONVCTRL(void)
{
	uint8_t convctrl = 0;

	vPortEnterCritical();

	i2c_read(I2C1, ADS7924_ADDRESS, 0x01, &convctrl);

	vPortExitCritical();

	return (convctrl & 0x40);
}

void ads7924_clrCONVCTRL(void)
{
	vPortEnterCritical();

	i2c_write(I2C1, ADS7924_ADDRESS, 0x13, 0b00000000);

	i2c_write(I2C1, ADS7924_ADDRESS, 0x13, 0b01000000);

	vPortExitCritical();
}

