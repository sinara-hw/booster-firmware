/*
 * channels.c
 *
 *  Created on: Oct 2, 2017
 *      Author: wizath
 */

#include "config.h"
#include "channels.h"
#include "ads7924.h"
#include "i2c.h"
#include "led_bar.h"
#include "math.h"
#include "locks.h"
#include "temp_mgt.h"
#include "eeprom.h"

static channel_t channels[8] = { 0 };
volatile uint8_t channel_mask = 0;

void rf_channels_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	channel_t ch;
	memset(&ch, 0, sizeof(channel_t));

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
	/* IN PINS */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
								  GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

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

void rf_channel_load_values(channel_t * ch)
{
	// load interlocks value
	ch->cal_values.input_dac_cal_value = eeprom_read16(DAC1_EEPROM_ADDRESS);
	ch->cal_values.output_dac_cal_value = eeprom_read16(DAC2_EEPROM_ADDRESS);
}

uint8_t rf_channels_detect(void)
{
	printf("Starting detect channels\n");

	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		for (int i = 0; i < CHANNEL_COUNT; i++)
		{
			i2c_mux_select(i);

			uint8_t dac_detected = i2c_device_connected(I2C1, I2C_DAC_ADDR);
			uint8_t dual_dac_detected = i2c_device_connected(I2C1, I2C_DUAL_DAC_ADDR);
			uint8_t temp_sensor_detected = i2c_device_connected(I2C1, I2C_MODULE_TEMP);

			if (dual_dac_detected && dac_detected && temp_sensor_detected)
			{
				printf("Channel %d OK\n", i);
				ads7924_init();
				channel_temp_sens_init();

				channel_mask |= 1UL << i;
				channels[i].detected = true;

				rf_channel_load_values(&channels[i]);
			}
		}

		lock_free(I2C_LOCK);
	}

	return 0;
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
	int bitmask = 1 << channel;

	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		i2c_mux_select(channel);
		i2c_dac_set(4095);

		// set calibration values
		// i2c_dual_dac_set(0, channels[channel].dac1_value);
		// i2c_dual_dac_set(1, channels[channel].dac2_value);

		lock_free(I2C_LOCK);
	}

	rf_channels_control(bitmask, true);

	vTaskDelay(300);
	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		i2c_mux_select(channel);
		i2c_dac_set_value(2.05f);

		lock_free(I2C_LOCK);
	}

	rf_channels_sigon(bitmask, true);
	vTaskDelay(100);
	rf_channels_sigon(bitmask, false);
	vTaskDelay(100);
	rf_channels_sigon(bitmask, true);

	return false;
}

void rf_channels_enable(uint8_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & mask) {
			rf_channel_enable_procedure(i);
			led_bar_or(1UL << i, 0, 0);
		}
	}
}

void rf_channels_disable(uint8_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & mask) {
			rf_channel_disable_procedure(i);
			led_bar_and(1UL << i, 0, 0);
		}
	}
}

void rf_channel_disable_procedure(uint8_t channel)
{
	int bitmask = 1 << channel;

	rf_channels_control(bitmask, false);
	rf_channels_sigon(bitmask, false);

	if (lock_take(I2C_LOCK, portMAX_DELAY)) {
		i2c_mux_select(channel);
		i2c_dac_set(0);
		i2c_dual_dac_set_val(0.0f, 0.0f);

		lock_free(I2C_LOCK);
	}
}

channel_t * rf_channel_get(uint8_t num)
{
	if (num < 8) return &channels[num];
	return NULL;
}

void channel_interlock_task(void *pvParameters)
{
	for (;;)
	{
		for (int i = 0; i < 8; i++) {
			if ((1 << i) & channel_mask) {




			}
		}

		vTaskDelay(10);
	}
}

void channel_measure_task(void *pvParameters)
{
	for (;;)
	{
		for (int i = 0; i < 8; i++) {
			if ((1 << i) & channel_mask) {




			}
		}

		vTaskDelay(100);
	}
}

void rf_clear_interlock(void)
{
	uint8_t ch_msk = (rf_channels_read_user() | rf_channels_read_ovl()) & rf_channels_read_enabled();

	rf_channels_sigon(ch_msk, false);

	if (lock_take(SPI_LOCK, 100)) {
		led_bar_and(ch_msk, 0xFF, 0xFF);
		lock_free(SPI_LOCK);
	}

	for (int i = 0; i < 8; i++) {
		channels[i].input_interlock = false;
		channels[i].output_interlock = false;
	}

	vTaskDelay(10);

	rf_channels_sigon(channel_mask & rf_channels_read_enabled(), true);

	if (lock_take(SPI_LOCK, 100)) {
		led_bar_write(rf_channels_read_sigon(), 0, 0);
		lock_free(SPI_LOCK);
	}
}
