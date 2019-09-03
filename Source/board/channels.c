/*
 * channels.c
 *
 *  Created on: Oct 2, 2017
 *      Author: wizath
 */

#include "config.h"
#include "channels.h"
#include "ads7924.h"
#include "max6642.h"
#include "i2c.h"
#include "led_bar.h"
#include "math.h"
#include "locks.h"
#include "temp_mgt.h"
#include "eeprom.h"
#include "tasks.h"
#include "ucli.h"
#include "device.h"

#define SW_EEPROM_VERSION			2

static channel_t channels[8] = { 0 };
volatile uint8_t channel_mask = 0;

TaskHandle_t task_rf_measure;
TaskHandle_t task_rf_info;
TaskHandle_t task_rf_interlock;

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

uint8_t rf_channels_get_mask(void)
{
	return channel_mask;
}

void rf_channel_load_values(channel_t * ch)
{
	uint8_t ver = eeprom_read(0x00);
	if (ver == 0xFF) eeprom_write(0x00, SW_EEPROM_VERSION);

	// load interlocks value
	ch->cal_values.input_dac_cal_value = eeprom_read16(DAC1_EEPROM_ADDRESS);
	ch->cal_values.output_dac_cal_value = eeprom_read16(DAC2_EEPROM_ADDRESS);

	ch->cal_values.fwd_pwr_offset = eeprom_read16(ADC1_OFFSET_ADDRESS);
	ch->cal_values.fwd_pwr_scale = eeprom_read16(ADC1_SCALE_ADDRESS);

	ch->cal_values.rfl_pwr_offset = eeprom_read16(ADC2_OFFSET_ADDRESS);
	ch->cal_values.rfl_pwr_scale = eeprom_read16(ADC2_SCALE_ADDRESS);

	ch->cal_values.bias_dac_cal_value = eeprom_read16(BIAS_DAC_VALUE_ADDRESS);

	// read the eeprom 6-byte UUID
	for (int i = 0xfa, j = 0; i <= 0xff; i++, j++) {
		ch->hwid[j] = eeprom_read(i);
	}

	uint32_t u32_int_scale = eeprom_read32(HW_INT_SCALE);
	uint32_t u32_int_offset = eeprom_read32(HW_INT_OFFSET);
	memcpy(&ch->cal_values.hw_int_scale, &u32_int_scale, sizeof(float));
	memcpy(&ch->cal_values.hw_int_offset, &u32_int_offset, sizeof(float));

	// load interlock setpoint in dBm
	ch->interlock_setpoint = (double) (ch->cal_values.output_dac_cal_value - ch->cal_values.hw_int_offset) / ch->cal_values.hw_int_scale;
}

uint8_t rf_channels_detect(void)
{
	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		for (int i = 0; i < CHANNEL_COUNT; i++)
		{
			i2c_mux_select(i);

			uint8_t dac_detected = i2c_device_connected(I2C1, I2C_DAC_ADDR);
			vTaskDelay(15);
			uint8_t dual_dac_detected = i2c_device_connected(I2C1, I2C_DUAL_DAC_ADDR);
			vTaskDelay(15);
			uint8_t temp_sensor_detected = i2c_device_connected(I2C1, I2C_MODULE_TEMP);
			vTaskDelay(15);

			if (dual_dac_detected && dac_detected && temp_sensor_detected)
			{
				ads7924_init();
				max6642_init();

				channel_mask |= 1UL << i;
				channels[i].detected = true;
				channels[i].input_interlock = false;
				channels[i].output_interlock = false;

				rf_channel_load_values(&channels[i]);

				ucli_log(UCLI_LOG_INFO, "RF Channel at %d detected, id = %02X:%02X\r\n", i, channels[i].hwid[4], channels[i].hwid[5]);
			} else {
				led_bar_or(0x00, 0x00, 1UL << i);
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
	device_t * dev;

	if (channels[channel].error) {
		led_bar_or(0, 0, (1UL << channel));
		return false;
	}

	// disable enabling power while DAC's are set to 0
	dev = device_get_config();
	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		i2c_mux_select(channel);
		i2c_dac_set(4095);

		lock_free(I2C_LOCK);
	}

	rf_channels_control(bitmask, true);
	vTaskDelay(10);

	if (lock_take(I2C_LOCK, portMAX_DELAY))
	{
		i2c_mux_select(channel);
		i2c_dac_set(4095);
		vTaskDelay(50);

		// set calibration values
		i2c_dual_dac_set(0, channels[channel].cal_values.input_dac_cal_value);
		vTaskDelay(10);
		i2c_dual_dac_set(1, channels[channel].cal_values.output_dac_cal_value);
		vTaskDelay(10);
		i2c_dac_set(channels[channel].cal_values.bias_dac_cal_value);
		vTaskDelay(10);

		lock_free(I2C_LOCK);
	}

	rf_channels_sigon(bitmask, true);
	vTaskDelay(5);
	rf_channels_sigon(bitmask, false);
	vTaskDelay(5);
	rf_channels_sigon(bitmask, true);

	vTaskDelay(50);

	return true;
}

void rf_channels_enable(uint8_t mask)
{
	for (int i = 0; i < 8; i++)
	{
		if ((1 << i) & mask) {
			if (rf_channel_enable_procedure(i)) {
				// dont light up led when interlock trips during poweron
				if (channels[i].sigon)
					led_bar_or(1UL << i, 0, 0);
			}
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
		i2c_dac_set(4095);

		lock_free(I2C_LOCK);
	}
}

channel_t * rf_channel_get(uint8_t num)
{
	if (num < 8) return &channels[num];
	return NULL;
}

void rf_channels_interlock_task(void *pvParameters)
{
	uint8_t channel_enabled = 0;
	uint8_t channel_ovl = 0;
	uint8_t channel_alert = 0;
	uint8_t channel_user = 0;
	uint8_t channel_sigon = 0;
	uint8_t err_cnt = 0;
	uint8_t err_cnt2 = 0;
	float pwr_diff = 0;
	uint8_t err_clear = 0;

	// since it's most crucial task,
	// we can feed the dog inside to avoid interrupts
	// because CPU get's really busy with VCP and SCPI
	IWDG_ReloadCounter();

	rf_channels_init();
	rf_channels_detect();

	xTaskCreate(rf_channels_measure_task, "CH MEAS", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 2, &task_rf_measure);
	xTaskCreate(rf_channels_info_task, "CH INFO", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 1, &task_rf_info);
	vTaskSuspend(task_rf_info);

	for (;;)
	{
//		GPIO_ToggleBits(BOARD_LED1);

		channel_enabled = rf_channels_read_enabled();
		channel_ovl = rf_channels_read_ovl();
		channel_alert = rf_channels_read_alert();
		channel_sigon = rf_channels_read_sigon();
		channel_user = rf_channels_read_user();

		for (int i = 0; i < 8; i++) {
			if ((1 << i) & channel_mask) {
				channels[i].enabled = (channel_enabled >> i) & 0x01;
				channels[i].sigon = (channel_sigon >> i) & 0x01;

				if (((channel_ovl >> i) & 0x01) && (channels[i].sigon && channels[i].enabled)) channels[i].input_interlock = true;
				if (((channel_user >> i) & 0x01) && (channels[i].sigon && channels[i].enabled)) channels[i].output_interlock = true;

				if ((channels[i].sigon && channels[i].enabled) && ( channels[i].output_interlock || channels[i].input_interlock)) {
					rf_channels_sigon(1 << i, false);

					ucli_log(UCLI_LOG_ERROR, "Interlock tripped on channel %d, i=%d o=%d\r\n", i, channels[i].input_interlock, channels[i].output_interlock);

					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(i);
						i2c_dac_set(4095);
						lock_free(I2C_LOCK);
					}

					led_bar_and((1UL << i), 0x00, 0x00);
					led_bar_or(0x00, (1UL << i), 0x00);
				}

				if (channels[i].enabled && (channels[i].measure.remote_temp > 60.0f || channels[i].measure.remote_temp < 5.0f))
				{
					// avoid one-type glitches on other channels
					err_clear = 0;
					if (err_cnt++ > 32) {
						rf_channel_disable_procedure(i);
						led_bar_and((1UL << i), 0x00, 0x00);
						led_bar_or(0, 0, (1UL << i));
						channels[i].error = 1;
						ucli_log(UCLI_LOG_ERROR, "Temperature error on channel %d temp = %0.2f, disabling\r\n", i, channels[i].measure.remote_temp);
						err_cnt = 0;
					}
				}

				if (channels[i].sigon && channels[i].enabled) {

					// protect against inf value after wrong reflected power calibration
					if (channels[i].measure.rfl_pwr >= 30.0 && channels[i].measure.rfl_pwr <= 80.0)
					{
						ucli_log(UCLI_LOG_ERROR, "Reverse power interlock tripped on channel %d, tx %0.2f rfl %0.2f\r\n", i, channels[i].measure.adc_ch1, channels[i].measure.adc_ch1);

						rf_channels_sigon(1 << i, false);
						if (lock_take(I2C_LOCK, portMAX_DELAY))
						{
							i2c_mux_select(i);
							i2c_dac_set(4095);
							lock_free(I2C_LOCK);
						}

						led_bar_and((1UL << i), 0x00, 0x00);
						led_bar_or(0x00, (1UL << i), 0x00);

						channels[i].soft_interlock = true;
					}
				}

				if (err_clear++ > 200)
				{
					err_cnt = 0;
					err_clear = 0;
				}
			}
		}

		vTaskDelay(10);
	}
}

void rf_channels_measure_task(void *pvParameters)
{
	device_t * dev;
	dev = device_get_config();

	for (;;)
	{
		for (int i = 0; i < 8; i++) {

			if ((1 << i) & channel_mask)
			{
				if (channels[i].enabled)
				{
					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(i);

						channels[i].measure.fwd_pwr = (double) (channels[i].measure.adc_raw_ch1 - channels[i].cal_values.fwd_pwr_offset) / (double) channels[i].cal_values.fwd_pwr_scale;

						// power meter clipping to avoid false readings below 5 dBm output power
						if (channels[i].measure.fwd_pwr < 5.0f)
							channels[i].measure.fwd_pwr = 5.0f;

						channels[i].measure.rfl_pwr = (double) (channels[i].measure.adc_raw_ch2 - channels[i].cal_values.rfl_pwr_offset) / (double) channels[i].cal_values.rfl_pwr_scale;

						channels[i].measure.i30 = (ads7924_get_channel_voltage(0) / dev->p30_gain) / dev->p30_current_sense;
						channels[i].measure.i60 = (ads7924_get_channel_voltage(1) / dev->p6_gain) / 0.1f;

						vTaskDelay(100);

//						printf("p30 %d p6v5 %d p5mp %d\r\n", ads7924_get_channel_data(0), ads7924_get_channel_data(1), ads7924_get_channel_data(3));

						channels[i].measure.p5v0mp = ads7924_get_channel_voltage(3) * 2.5f;

						lock_free(I2C_LOCK);
					}
				} else {
					// avoid unnecessary confusion when channel is disabled
					// and providing false measurements
					channels[i].measure.fwd_pwr = 0;
					channels[i].measure.rfl_pwr = 0;
					channels[i].measure.i30 = 0;
					channels[i].measure.i60 = 0;
					channels[i].measure.p5v0mp = 0;

					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(i);
						channels[i].measure.local_temp = max6642_get_local_temp();
						channels[i].measure.remote_temp = max6642_get_remote_temp();

						lock_free(I2C_LOCK);
					}
				}
			}
		}

		vTaskDelay(50);
	}
}

void rf_channels_hwint_override(uint8_t channel, double int_value)
{
	if (channel < 8)
	{
		if (int_value >= -10.0f && int_value < 39.0f)
		{
			double value = (double) ((int_value * channels[channel].cal_values.fwd_pwr_scale) + channels[channel].cal_values.fwd_pwr_offset);
			uint16_t dac_value = (uint16_t) value;
		}
	}
}

void rf_clear_interlock(void)
{
	for (int i = 0; i < 8; i++) {
		if (!channels[i].error && channels[i].detected) {
			rf_channel_clear_interlock(i);
		}
	}
}

bool rf_channel_interlock_set(uint8_t channel, double value)
{
	device_t * dev;
	channel_t * ch;
	dev = device_get_config();
	uint16_t dac_value = 0;

	if (channel < 8) {
		ch = rf_channel_get(channel);
		dac_value = (uint16_t) ((ch->cal_values.hw_int_scale * value) + ch->cal_values.hw_int_offset);

		ch->cal_values.output_dac_cal_value = dac_value;

		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			eeprom_write16(DAC2_EEPROM_ADDRESS, dac_value);
			if (ch->enabled) {
				i2c_dual_dac_set(1, ch->cal_values.output_dac_cal_value);
			}
			lock_free(I2C_LOCK);
			printf("[intv] Interlock value for %0.2f = %d\r\n", value, ch->cal_values.output_dac_cal_value);
		}

		return true;
	}

	return false;
}

double rf_channel_interlock_get(uint8_t channel)
{
	device_t * dev;
	channel_t * ch;
	dev = device_get_config();

	if (channel < 8) {
		ch = rf_channel_get(channel);
		double value = (double) (ch->cal_values.output_dac_cal_value - ch->cal_values.hw_int_offset) / ch->cal_values.hw_int_scale;
		return value;
	}

	return 0.0f;
}

bool rf_channel_clear_interlock(uint8_t channel)
{
	channel_t * ch;

	if (channel < 8) {
		ch = rf_channel_get(channel);
		if (ch->detected) {
			ch->input_interlock = false;
			ch->output_interlock = false;
			ch->soft_interlock = false;

			vTaskDelay(10);

			uint8_t chan = rf_channels_read_enabled() & (1 << channel);
			uint8_t channel_mask = rf_channels_get_mask() & chan;

			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select(channel);
				i2c_dac_set(4095);
				vTaskDelay(10);
				i2c_dac_set(channels[channel].cal_values.bias_dac_cal_value);

				lock_free(I2C_LOCK);
			}

			rf_channels_sigon(channel_mask, false);
			vTaskDelay(50);
			rf_channels_sigon(channel_mask, true);

			led_bar_or(rf_channels_read_sigon(), 0, 0);
			led_bar_and(0x00, (1 << channel), 0x00);

			return true;
		}
	}

	return false;
}

void rf_channels_info_task(void *pvParameters)
{
	for (;;)
	{
		puts("\033[2J");
		printf("PGOOD: %d\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4));

		printf("FAN SPEED: %d %%\n", temp_mgt_get_fanspeed());
		printf("AVG TEMP: %0.2f CURRENT: %0.2f\n", temp_mgt_get_avg_temp(), temp_mgt_get_max_temp());
		printf("CHANNELS INFO\n");
		printf("==============================================================================\n");
		printf("\t\t#0\t#1\t#2\t#3\t#4\t#5\t#6\t#7\n");

		printf("DETECTED\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].detected,
																channels[1].detected,
																channels[2].detected,
																channels[3].detected,
																channels[4].detected,
																channels[5].detected,
																channels[6].detected,
																channels[7].detected);

		printf("HWID\t\t%02X:%02X\t%02X:%02X\t%02X:%02X\t%02X:%02X\t%02X:%02X\t%02X:%02X\t%02X:%02X\t%02X:%02X\t\n",
																channels[0].hwid[4],
																channels[0].hwid[5],
																channels[1].hwid[4],
																channels[1].hwid[5],
																channels[2].hwid[4],
																channels[2].hwid[5],
																channels[3].hwid[4],
																channels[3].hwid[5],
																channels[4].hwid[4],
																channels[4].hwid[5],
																channels[5].hwid[4],
																channels[5].hwid[5],
																channels[6].hwid[4],
																channels[6].hwid[5],
																channels[7].hwid[4],
																channels[7].hwid[5]);

		printf("TXPWR [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].measure.adc_ch1,
																						channels[1].measure.adc_ch1,
																						channels[2].measure.adc_ch1,
																						channels[3].measure.adc_ch1,
																						channels[4].measure.adc_ch1,
																						channels[5].measure.adc_ch1,
																						channels[6].measure.adc_ch1,
																						channels[7].measure.adc_ch1);

		printf("RFLPWR [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].measure.adc_ch2,
																						channels[1].measure.adc_ch2,
																						channels[2].measure.adc_ch2,
																						channels[3].measure.adc_ch2,
																						channels[4].measure.adc_ch2,
																						channels[5].measure.adc_ch2,
																						channels[6].measure.adc_ch2,
																						channels[7].measure.adc_ch2);

		printf("TXPWR [dBm]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].measure.fwd_pwr,
																						channels[1].measure.fwd_pwr,
																						channels[2].measure.fwd_pwr,
																						channels[3].measure.fwd_pwr,
																						channels[4].measure.fwd_pwr,
																						channels[5].measure.fwd_pwr,
																						channels[6].measure.fwd_pwr,
																						channels[7].measure.fwd_pwr);

		printf("RFLPWR [dBm]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].measure.rfl_pwr,
																						channels[1].measure.rfl_pwr,
																						channels[2].measure.rfl_pwr,
																						channels[3].measure.rfl_pwr,
																						channels[4].measure.rfl_pwr,
																						channels[5].measure.rfl_pwr,
																						channels[6].measure.rfl_pwr,
																						channels[7].measure.rfl_pwr);

		printf("I30V [A]\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t\n", channels[0].measure.i30,
																						channels[1].measure.i30,
																						channels[2].measure.i30,
																						channels[3].measure.i30,
																						channels[4].measure.i30,
																						channels[5].measure.i30,
																						channels[6].measure.i30,
																						channels[7].measure.i30);

		printf("I6V0 [A]\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t\n", channels[0].measure.i60,
																						channels[1].measure.i60,
																						channels[2].measure.i60,
																						channels[3].measure.i60,
																						channels[4].measure.i60,
																						channels[5].measure.i60,
																						channels[6].measure.i60,
																						channels[7].measure.i60);

		printf("5V0MP [V]\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t%0.3f\t\n", channels[0].measure.p5v0mp,
																						channels[1].measure.p5v0mp,
																						channels[2].measure.p5v0mp,
																						channels[3].measure.p5v0mp,
																						channels[4].measure.p5v0mp,
																						channels[5].measure.p5v0mp,
																						channels[6].measure.p5v0mp,
																						channels[7].measure.p5v0mp);

		printf("ON\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].enabled,
															channels[1].enabled,
															channels[2].enabled,
															channels[3].enabled,
															channels[4].enabled,
															channels[5].enabled,
															channels[6].enabled,
															channels[7].enabled);

		printf("SON\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].sigon,
															channels[1].sigon,
															channels[2].sigon,
															channels[3].sigon,
															channels[4].sigon,
															channels[5].sigon,
															channels[6].sigon,
															channels[7].sigon);

		printf("IINT\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].input_interlock,
															channels[1].input_interlock,
															channels[2].input_interlock,
															channels[3].input_interlock,
															channels[4].input_interlock,
															channels[5].input_interlock,
															channels[6].input_interlock,
															channels[7].input_interlock);

		printf("OINT\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].output_interlock,
															channels[1].output_interlock,
															channels[2].output_interlock,
															channels[3].output_interlock,
															channels[4].output_interlock,
															channels[5].output_interlock,
															channels[6].output_interlock,
															channels[7].output_interlock);

		printf("SINT\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].soft_interlock,
															channels[1].soft_interlock,
															channels[2].soft_interlock,
															channels[3].soft_interlock,
															channels[4].soft_interlock,
															channels[5].soft_interlock,
															channels[6].soft_interlock,
															channels[7].soft_interlock);

		printf("ADC1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].measure.adc_raw_ch1,
																channels[1].measure.adc_raw_ch1,
																channels[2].measure.adc_raw_ch1,
																channels[3].measure.adc_raw_ch1,
																channels[4].measure.adc_raw_ch1,
																channels[5].measure.adc_raw_ch1,
																channels[6].measure.adc_raw_ch1,
																channels[7].measure.adc_raw_ch1);

		printf("ADC2\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].measure.adc_raw_ch2,
															channels[1].measure.adc_raw_ch2,
															channels[2].measure.adc_raw_ch2,
															channels[3].measure.adc_raw_ch2,
															channels[4].measure.adc_raw_ch2,
															channels[5].measure.adc_raw_ch2,
															channels[6].measure.adc_raw_ch2,
															channels[7].measure.adc_raw_ch2);

		printf("INTSET [dBm]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].interlock_setpoint,
																					channels[1].interlock_setpoint,
																					channels[2].interlock_setpoint,
																					channels[3].interlock_setpoint,
																					channels[4].interlock_setpoint,
																					channels[5].interlock_setpoint,
																					channels[6].interlock_setpoint,
																					channels[7].interlock_setpoint);

		printf("DAC1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].cal_values.input_dac_cal_value,
																channels[1].cal_values.input_dac_cal_value,
																channels[2].cal_values.input_dac_cal_value,
																channels[3].cal_values.input_dac_cal_value,
																channels[4].cal_values.input_dac_cal_value,
																channels[5].cal_values.input_dac_cal_value,
																channels[6].cal_values.input_dac_cal_value,
																channels[7].cal_values.input_dac_cal_value);

		printf("DAC2\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].cal_values.output_dac_cal_value,
															channels[1].cal_values.output_dac_cal_value,
															channels[2].cal_values.output_dac_cal_value,
															channels[3].cal_values.output_dac_cal_value,
															channels[4].cal_values.output_dac_cal_value,
															channels[5].cal_values.output_dac_cal_value,
															channels[6].cal_values.output_dac_cal_value,
															channels[7].cal_values.output_dac_cal_value);

		printf("SCALE1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].cal_values.fwd_pwr_scale,
															channels[1].cal_values.fwd_pwr_scale,
															channels[2].cal_values.fwd_pwr_scale,
															channels[3].cal_values.fwd_pwr_scale,
															channels[4].cal_values.fwd_pwr_scale,
															channels[5].cal_values.fwd_pwr_scale,
															channels[6].cal_values.fwd_pwr_scale,
															channels[7].cal_values.fwd_pwr_scale);

		printf("OFFSET1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].cal_values.fwd_pwr_offset,
															channels[1].cal_values.fwd_pwr_offset,
															channels[2].cal_values.fwd_pwr_offset,
															channels[3].cal_values.fwd_pwr_offset,
															channels[4].cal_values.fwd_pwr_offset,
															channels[5].cal_values.fwd_pwr_offset,
															channels[6].cal_values.fwd_pwr_offset,
															channels[7].cal_values.fwd_pwr_offset);

		printf("BIASCAL\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].cal_values.bias_dac_cal_value,
																	channels[1].cal_values.bias_dac_cal_value,
																	channels[2].cal_values.bias_dac_cal_value,
																	channels[3].cal_values.bias_dac_cal_value,
																	channels[4].cal_values.bias_dac_cal_value,
																	channels[5].cal_values.bias_dac_cal_value,
																	channels[6].cal_values.bias_dac_cal_value,
																	channels[7].cal_values.bias_dac_cal_value);

		printf("HWIS\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].cal_values.hw_int_scale,
																			channels[1].cal_values.hw_int_scale,
																			channels[2].cal_values.hw_int_scale,
																			channels[3].cal_values.hw_int_scale,
																			channels[4].cal_values.hw_int_scale,
																			channels[5].cal_values.hw_int_scale,
																			channels[6].cal_values.hw_int_scale,
																			channels[7].cal_values.hw_int_scale);

		printf("HWIO\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].cal_values.hw_int_offset,
															channels[1].cal_values.hw_int_offset,
															channels[2].cal_values.hw_int_offset,
															channels[3].cal_values.hw_int_offset,
															channels[4].cal_values.hw_int_offset,
															channels[5].cal_values.hw_int_offset,
															channels[6].cal_values.hw_int_offset,
															channels[7].cal_values.hw_int_offset);

		printf("LTEMP\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].measure.local_temp,
																			channels[1].measure.local_temp,
																			channels[2].measure.local_temp,
																			channels[3].measure.local_temp,
																			channels[4].measure.local_temp,
																			channels[5].measure.local_temp,
																			channels[6].measure.local_temp,
																			channels[7].measure.local_temp);

		printf("RTEMP\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].measure.remote_temp,
																				channels[1].measure.remote_temp,
																				channels[2].measure.remote_temp,
																				channels[3].measure.remote_temp,
																				channels[4].measure.remote_temp,
																				channels[5].measure.remote_temp,
																				channels[6].measure.remote_temp,
																				channels[7].measure.remote_temp);

		printf("==============================================================================\n");

		vTaskDelay(1000);
	}
}

uint16_t rf_channel_calibrate_input_interlock_v3(uint8_t channel, int16_t start_value, uint8_t step)
{
	int16_t dacval = start_value;
	uint8_t count = 0;

	if (step == 0) step = 1;

	if (channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select(channel);
			i2c_dual_dac_set(0, dacval);

			lock_free(I2C_LOCK);
		}

		vTaskSuspend(task_rf_interlock);
		vTaskDelay(100);
		rf_channel_enable_procedure(channel);
		vTaskDelay(100);
		rf_clear_interlock();

		while (dacval > 0) {
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select(channel);
				i2c_dual_dac_set(0, dacval);

				lock_free(I2C_LOCK);
			}

//			if (dacval == 1500 || dacval == start_value) {
//				// clear the issue of interlock popping
//				// straight up after setting first value
//				vTaskDelay(100);
//				rf_clear_interlock();
//			}

			vTaskDelay(200);

			if (count++ % 2)
				led_bar_or((1 << channel), 0, 0);
			else
				led_bar_and((1 << channel), 0, 0);

			uint8_t val = 0;
			val = rf_channels_read_ovl();
			printf("[iintcal] trying value %d, status %d\n", dacval, (val >> channel) & 0x01);

			if ((val >> channel) & 0x01) {
				printf("[iintcal] done interlock value = %d\r\n", dacval);

				rf_channel_disable_procedure(channel);
				vTaskDelay(200);
				rf_clear_interlock();
				vTaskDelay(100);
				vTaskResume(task_rf_interlock);
				led_bar_and((1 << channel), 0, 0);

				return (uint16_t) dacval;
				break;
			}

			vTaskDelay(200);
			dacval -= step;
		}

		rf_channel_disable_procedure(channel);
		vTaskDelay(100);
		vTaskResume(task_rf_interlock);
		led_bar_and((1 << channel), 0, 0);

//		printf("[iintcal] error, interlock value not found\n");
	}

	return (uint16_t) dacval;
}

uint16_t rf_channel_calibrate_output_interlock_v3(uint8_t channel, int16_t start_value, uint8_t step)
{
	int16_t dacval = start_value;
	uint8_t count = 0;

	if (step == 0) step = 1;

	if (channel < 8)
	{
		channels[channel].cal_values.output_dac_cal_value = dacval;

		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select(channel);
			i2c_dual_dac_set(1, 0);

			lock_free(I2C_LOCK);
		}

		vTaskSuspend(task_rf_interlock);
		vTaskDelay(100);
		rf_channel_enable_procedure(channel);
		vTaskDelay(100);
		rf_clear_interlock();

		while (dacval > 0) {
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select(channel);
				i2c_dual_dac_set(1, dacval);

				lock_free(I2C_LOCK);
			}

//			if (dacval == 1500 || dacval == start_value) {
//				// clear the issue of interlock popping
//				// straight up after setting first value
//				vTaskDelay(200);
//				rf_clear_interlock();
//			}

			vTaskDelay(200);

			if (count++ % 2)
				led_bar_or((1 << channel), 0, 0);
			else
				led_bar_and((1 << channel), 0, 0);

			uint8_t val = 0;
			val = rf_channels_read_user();
			printf("[ointcal] trying value %d, status %d\n", dacval, (val >> channel) & 0x01);

			if ((val >> channel) & 0x01) {
				printf("[ointcal] done, interlock value = %d\n", dacval);
				rf_channel_disable_procedure(channel);
				vTaskDelay(100);
				vTaskResume(task_rf_interlock);
				led_bar_and((1 << channel), 0, 0);

				return (uint16_t) dacval;
				break;
			}

			vTaskDelay(200);
			dacval -= step;
		}

		rf_channel_disable_procedure(channel);
		vTaskDelay(100);
		vTaskResume(task_rf_interlock);
		led_bar_and((1 << channel), 0, 0);

		printf("[ointcal] error, interlock value not found\n");
	}

	return (uint16_t) dacval;
}

bool rf_channel_calibrate_bias(uint8_t channel, uint16_t current)
{
	int16_t dacval = 4095;
	uint8_t count = 0;
	uint16_t value;
	double avg_current = 0.0f;
	device_t * dev = device_get_config();

	if (channel < 8)
	{
		vTaskSuspend(task_rf_interlock);
		vTaskDelay(100);
		rf_channel_enable_procedure(channel);
		rf_channels_sigon((1 << channel), false); // disable sigon

		vTaskDelay(100);

		while (dacval > 0)
		{
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select(channel);
				i2c_dac_set(dacval);

				lock_free(I2C_LOCK);
			}

			vTaskDelay(100);

			avg_current = 0.0f;
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select(channel);
				for (int i = 0; i < 32; i++)
				{
					float value = (((ads7924_get_channel_voltage(0) / dev->p30_gain) / dev->p30_current_sense) * 1000);
					avg_current += value;
					vTaskDelay(20);
				}
				lock_free(I2C_LOCK);
			}

			value = (uint16_t) (avg_current / 32);
			uint16_t diff = abs(value - current);

			// guard for off-the-chart values like 200ma's at start
			if (value > (float) (current * 1.02f))
			{
				rf_channel_disable_procedure(channel);
				vTaskResume(task_rf_interlock);
				led_bar_and((1 << channel), 0, 0);

				printf("[biascal] error, current too high %d > %d\r\n", value, current);
				return false;
			}

			if (count++ % 2)
				led_bar_or((1 << channel), 0, 0);
			else
				led_bar_and((1 << channel), 0, 0);

//			printf("[biascal] dacval %d, progress %d/%d, diff %d, step %d\r\n", dacval, value, current, diff, diff > 10 ? (diff * 2) : 15);
			ucli_progress_bar(value, 0, current, true);

			if (value > current * 0.98 && value < current * 1.02)
			{
				rf_channel_disable_procedure(channel);
				vTaskResume(task_rf_interlock);
				led_bar_and((1 << channel), 0, 0);

				eeprom_write16(BIAS_DAC_VALUE_ADDRESS, dacval);
				channels[channel].cal_values.bias_dac_cal_value = dacval;

				ucli_progress_bar(value, 0, value, false);
				printf("[biascal] done, value = %d, current = %d\n", dacval, value);
				return true;
			}

			dacval -= diff > 10 ? (diff * 2) : 10;
		}
	}

	printf("[biascal] error, current too low %d < %d\r\n", value, current);
	rf_channel_disable_procedure(channel);
	vTaskResume(task_rf_interlock);
	led_bar_and((1 << channel), 0, 0);

	return false;
}
