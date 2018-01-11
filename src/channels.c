/*
 * channels.c
 *
 *  Created on: Oct 2, 2017
 *      Author: wizath
 */

#include "channels.h"
#include "ads7924.h"
#include "i2c.h"
#include "led_bar.h"

#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0')

static uint8_t available_channels[] = {7, 8};
static uint8_t channel_count = 2;

channel_t channels[8] = { 0 };

uint8_t rf_available_channels(uint8_t * arr)
{
	memcpy(arr, available_channels, channel_count);
	return channel_count;
}

void rf_channels_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	channel_t ch = {
		.enabled = 0,
		.overvoltage = 0,
		.alert = 0,
		.userio = 0,
		.sigon = 0,
		.adc_ch1 = 0,
		.adc_ch2 = 0,
		.adc_raw_ch1 = 0,
		.adc_raw_ch1 = 0,
		.pwr_ch1 = 0,
		.pwr_ch2 = 0,
		.pwr_ch3 = 0,
		.pwr_ch4 = 0,
	};

	for (int i = 0; i < 8; i++) {
		channels[i] = ch;
	}

	/* Channel ENABLE Pins */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
								  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Disable all channels by default */
	GPIO_ResetBits(GPIOD, 0b0000000011111111);

	/* Channel ALERT Pins */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |
				                  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* Channel OVL Pins */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |
								  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	/* Channel USER IO */
	/* OUT PINS */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
								  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

//	/* Disable all USER IO by default */
//	GPIO_ResetBits(GPIOE, 0b0000000011111111);

	/* Channel SIG_ON */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 |
								  GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	/* Disable all SIGON IO by default */
	GPIO_ResetBits(GPIOG, 0b1111111100000000);

	/* Channel ADCs RESET */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* ADC reset off */
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
}

void rf_channels_control(uint16_t channel_mask, bool enable)
{
	/* ENABLE Pins GPIOD 0-7 */
	if (enable)
		GPIO_SetBits(GPIOD, channel_mask);
	else
		GPIO_ResetBits(GPIOD, channel_mask);
}

void rf_channels_sigon(uint16_t channel_mask, bool enable)
{
	/* ENABLE Pins GPIOD 0-7 */
	if (enable)
		GPIO_SetBits(GPIOG, (channel_mask << 8) & 0xFF00);
	else
		GPIO_ResetBits(GPIOG, (channel_mask << 8) & 0xFF00);
}

uint8_t rf_channels_read_alert(void)
{
	/* ALERT Pins GPIOD 8-15 */
	return (GPIO_ReadInputData(GPIOD) & 0xFF00) >> 8;
}

uint8_t rf_channels_read_user(void)
{
	/* ALERT Pins GPIOD 8-15 */
	return (GPIO_ReadInputData(GPIOE) & 0xFF);
}

uint8_t rf_channels_read_sigon(void)
{
	/* ENABLE Pins GPIOG 8-15 */
	return (GPIO_ReadOutputData(GPIOG) & 0xFF00) >> 8;
}

uint8_t rf_channels_read_ovl(void)
{
	/* OVL Pins GPIOE 8-15 */
	return (GPIO_ReadInputData(GPIOE) & 0xFF00) >> 8;
}

uint8_t rf_channels_read_enabled(void)
{
	/* ENABLE Pins GPIOD 0-8 */
	return (GPIO_ReadOutputData(GPIOD) & 0xFF);
}

bool rf_channel_enable_procedure(uint8_t channel)
{
//	uint8_t alert, ovl, pgood;

	taskENTER_CRITICAL();
	i2c_mux_select(channel);
	i2c_dac_set(4095);
	taskEXIT_CRITICAL();

	rf_channels_control((2 ^ channel) - 1, true);

	vTaskDelay(300);

	taskENTER_CRITICAL();
	i2c_mux_select(channel);
	i2c_dac_set(0);
	taskEXIT_CRITICAL();
	vTaskDelay(300);

	return false;
}

void rf_channels_enable(uint8_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & mask)
			rf_channel_enable_procedure(i);
	}
}

void rf_channels_disable(uint8_t channel)
{
	rf_channels_control((2 ^ channel) - 1, false);
	rf_channels_sigon((2 ^ channel) - 1, false);

	taskENTER_CRITICAL();
	i2c_mux_select(channel);
	i2c_dac_set(0);
	taskEXIT_CRITICAL();
}

void rf_sigon_enable(uint8_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & mask)
			rf_channels_sigon((2 ^ i) - 1, true);
	}
}

void rf_disable_dac(void)
{
	for (int i = 0; i < 8; i++)
	{
		taskENTER_CRITICAL();
		i2c_mux_select(i);
		i2c_dac_set(0);
		taskEXIT_CRITICAL();
	}
}

void prcRFChannelsTask(void *pvParameters)
{
	led_bar_init();
	led_bar_write(255, 255, 255);
	rf_channels_init();

	for (int ch = 0; ch < 8; ch++) {
		i2c_mux_select(ch);
		printf("[log] i2c bus scanned, found %d devices\n", i2c_scan_devices(false));
		ads7924_init();
	}

	led_bar_write(0, 0, 0);

	uint8_t channel_enabled = 0;
	uint8_t channel_ovl = 0;
	uint8_t channel_alert = 0;
	uint8_t channel_user = 0;
	uint8_t channel_sigon = 0;

	for (;;)
	{
		GPIO_ToggleBits(BOARD_LED1);

		channel_enabled = rf_channels_read_enabled();
		channel_ovl = rf_channels_read_ovl();
		channel_alert = rf_channels_read_alert();
		channel_sigon = rf_channels_read_sigon();
		channel_user = rf_channels_read_user();

		for (int i = 0; i < 8; i++)
		{
			channels[i].alert = (channel_alert >> i) & 0x01;
			channels[i].enabled = (channel_enabled >> i) & 0x01;
			channels[i].overvoltage = (channel_ovl >> i) & 0x01;
			channels[i].sigon = (channel_sigon >> i) & 0x01;
			channels[i].userio = (channel_user >> i) & 0x01;

			if (channels[i].sigon && ( channels[i].userio || channels[i].overvoltage )) {
				rf_channels_disable(i);
				led_bar_write(rf_channels_read_sigon(), (channel_ovl | channel_user) & channel_sigon, 0);
			}

			i2c_mux_select(i);
			channels[i].pwr_ch1 = ads7924_get_channel_data(0);
			channels[i].pwr_ch2 = ads7924_get_channel_data(1);
			channels[i].pwr_ch3 = ads7924_get_channel_data(2);
			channels[i].pwr_ch4 = ads7924_get_channel_data(3);
		}

		vTaskDelay(10);
	}
}
