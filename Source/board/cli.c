/*
 * cli.c
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */

#include "cli.h"

#include "network.h"
#include "usb.h"
#include "i2c.h"
#include "channels.h"
#include "locks.h"
#include "eeprom.h"
#include "led_bar.h"
#include "tasks.h"
#include "ads7924.h"
#include "math.h"
#include "device.h"

extern xQueueHandle _xRxQueue;
extern int __io_putchar(int ch);
static ucli_cmd_t g_cmds[];

static void fh_reboot(void * a_data)
{
	NVIC_SystemReset();
}


static void fh_start(void * a_data)
{
	vTaskResume(task_rf_info);
}


static void fh_stop(void * a_data)
{
	vTaskSuspend(task_rf_info);
}


static void fh_sw_version(void * a_data)
{
	printf("RFPA v%.02f, built %s %s, for hardware revision: v%0.2f\r\n", 1.31f, __DATE__, __TIME__, 1.1f);
}

static void fh_enabled(void * a_data)
{
	printf("[enabled] %d %d\r\n", rf_channels_read_enabled(), rf_channels_read_sigon());
}

static void fh_powercfg(void * a_data)
{
	int cfg = 0;
	device_t * dev = device_get_config();

	if (ucli_param_get_int(1, &cfg))
	{
		cfg = (uint8_t) cfg;
		eeprom_write_mb(POWERCFG_STATUS, cfg);
		dev->powercfg = cfg;
		printf("[powercfg] set %d\r\n", dev->powercfg);
	} else {
		printf("[powercfg] %d\r\n", dev->powercfg);
	}
}

static void fh_fanlevel(void * a_data)
{
	int cfg = 0;
	device_t * dev = device_get_config();

	if (ucli_param_get_int(1, &cfg))
	{
		cfg = (uint8_t) cfg;
		eeprom_write_mb(FAN_MINIMUM_LEVEL, cfg);
		dev->fan_level = cfg;
		printf("[fanlevel] set %d\r\n", dev->fan_level);
	} else {
		printf("[fanlevel] %d\r\n", dev->fan_level);
	}
}

static void fh_status(void * a_data)
{
	int channel = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	channel = (uint8_t) channel;

	if (channel < 8) {
		ch = rf_channel_get(channel);

		printf("[status] e=%d s=%d r1=%d r2=%d tx=%0.3f rf=%0.3f curr=%0.3f t=%0.2f\r\n", ch->enabled,
				ch->sigon, ch->measure.adc_raw_ch1, ch->measure.adc_raw_ch2,
				ch->measure.fwd_pwr, ch->measure.rfl_pwr, ch->measure.i30, ch->measure.remote_temp);

	} else
		printf("[status] Wrong channel number\r\n");
}

static void fh_currents(void * a_data)
{
	channel_t * ch;
	double currents[8] = { 0 };
	for (int i = 0; i < 8; i++)
	{
		ch = rf_channel_get(i);
		if (ch->detected)
		{
			currents[i] = ch->measure.i30;
		} else {
			currents[i] = 0.0f;
		}
	}

	printf("[currents] %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f, %0.4f\r\n",
			currents[0],currents[1],currents[2],currents[3],currents[4],currents[5],currents[6],currents[7]);
}

static void fh_clearint(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	int channel = 0;

	ucli_param_get_int(1, &channel);
	channel = (uint8_t) channel;

	if (a_ctx->argc == 1) {
		rf_clear_interlock();
		printf("[clear] cleared all\r\n");
	} else {
		if (rf_channel_clear_interlock(channel))
			printf("[clear] cleared %d\r\n", channel);
		else
			printf("[clear] Wrong channel number\r\n");
	}
}

static void fh_devid(void * a_data)
{
	unsigned int b1 = (*(uint32_t *) (0x1FFF7A10));
	unsigned int b2 = (*(uint32_t *) (0x1FFF7A10 + 4));
	unsigned int b3 = (*(uint32_t *) (0x1FFF7A10 + 8));
	printf("[devid] %08X%08X%08X\r\n", b1, b2, b3);
}

static void fh_bootloader(void * a_data)
{
	usb_enter_bootloader();
}

static void fh_detected(void * a_data)
{
	printf("[detected] %d\r\n", rf_channels_get_mask());
}

static void fh_i2cd(void * a_data)
{
	int channel = 0;

	ucli_param_get_int(1, &channel);
	channel = (uint8_t) channel;

	if (lock_take(I2C_LOCK, portMAX_DELAY)) {
		i2c_mux_select(channel);

		i2c_scan_devices(true);
		lock_free(I2C_LOCK);
	}
}


static uint16_t cal_in_val1 = 0;
static uint16_t cal_in_pwr1 = 0;
static uint16_t cal_in_val2 = 0;
static uint16_t cal_in_pwr2 = 0;

static uint16_t cal_rfl_val1 = 0;
static uint16_t cal_rfl_pwr1 = 0;
static uint16_t cal_rfl_val2 = 0;
static uint16_t cal_rfl_pwr2 = 0;


static void fh_calpwr(void * a_data)
{
	int channel = 0;
	int pwr_type = 0;
	int pwr_cal = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &pwr_type);
	ucli_param_get_int(3, &pwr_cal);

	channel = (uint8_t) channel;
	pwr_type = (uint8_t) pwr_type;
	pwr_cal = (uint8_t) pwr_cal;

	if (channel < 8) {

		ch = rf_channel_get(channel);

		if (pwr_type == 1) {
			printf("[calpwr] done, value1 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch1);
			cal_in_pwr1 = pwr_cal;
			cal_in_val1 = ch->measure.adc_raw_ch1;
		} else if (pwr_type == 2) {
			printf("[calpwr] done, value2 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch1);
			cal_in_pwr2 = pwr_cal;
			cal_in_val2 = ch->measure.adc_raw_ch1;
		} else if (pwr_type == 3) {
			printf("[calpwr] done, value1 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch2);
			cal_rfl_pwr1 = pwr_cal;
			cal_rfl_val1 = ch->measure.adc_raw_ch2;
		} else if (pwr_type == 4) {
			printf("[calpwr] done, value2 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch2);
			cal_rfl_pwr2 = pwr_cal;
			cal_rfl_val2 = ch->measure.adc_raw_ch2;
		}

		if (cal_in_val1 != 0 && cal_in_val2 != 0)
		{
//			printf("[calwpr] Calculating points for %d -> %d\n", cal_in_pwr1, cal_in_pwr2);
			uint16_t a = ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2));
			uint16_t b = (cal_in_val1 - ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2)) * cal_in_pwr1);
			printf("[calpwr] done, a=%d b=%d\n", a, b);

			ch = rf_channel_get(channel);
			// save values to eeprom
			ch->cal_values.fwd_pwr_offset = b;

			if (lock_take(I2C_LOCK, portMAX_DELAY)) {
				i2c_mux_select(channel);

				eeprom_write16(ADC1_OFFSET_ADDRESS, b);
				vTaskDelay(100);
				eeprom_write16(ADC1_SCALE_ADDRESS, a);
				lock_free(I2C_LOCK);
			}

			ch->cal_values.fwd_pwr_scale = a;
			cal_in_val1 = 0;
			cal_in_pwr1 = 0;
			cal_in_val2 = 0;
			cal_in_pwr2 = 0;
		}

		if (cal_rfl_val1 != 0 && cal_rfl_val2 != 0)
		{
//			printf("[calwpr] Calculating points for %d -> %d\n", cal_rfl_pwr1, cal_rfl_pwr2);
			int16_t a = ((cal_rfl_val1 - cal_rfl_val2) / (cal_rfl_pwr1 - cal_rfl_pwr2));
			int16_t b = (cal_rfl_val1 - ((cal_rfl_val1 - cal_rfl_val2) / (cal_rfl_pwr1 - cal_rfl_pwr2)) * cal_rfl_pwr1);
			printf("[calpwr] done, a=%d b=%d\n", a, b);

			ch = rf_channel_get(channel);
			// save values to eeprom
			ch->cal_values.rfl_pwr_offset = a;
			ch->cal_values.rfl_pwr_scale = b;

			if (lock_take(I2C_LOCK, portMAX_DELAY)) {
				i2c_mux_select(channel);

				eeprom_write16(ADC2_OFFSET_ADDRESS, b);
				vTaskDelay(100);
				eeprom_write16(ADC2_SCALE_ADDRESS, a);
				lock_free(I2C_LOCK);
			}

			cal_rfl_val1 = 0;
			cal_rfl_pwr1 = 0;
			cal_rfl_val2 = 0;
			cal_rfl_pwr2 = 0;
		}

	} else
		printf("[calwpr] Wrong channel number\r\n");
}


static uint16_t int_cal_val_s = 0;
static uint16_t int_cal_pwr_s = 0;
static uint16_t int_cal_val_e = 0;
static uint16_t int_cal_pwr_e = 0;


static void fh_intcal(void * a_data)
{
	int channel = 0;
	int pwr_type = 0;
	int pwr_cal = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &pwr_type);
	ucli_param_get_int(3, &pwr_cal);

	channel = (uint8_t) channel;
	pwr_type = (uint8_t) pwr_type;
	pwr_cal = (uint8_t) pwr_cal;

	uint16_t retval = 0;
	uint16_t dacval = 4095;

	if (channel < 8) {
		ch = rf_channel_get(channel);
		uint16_t old_dac1 = ch->cal_values.input_dac_cal_value;
		uint16_t old_dac2 = ch->cal_values.output_dac_cal_value;

		printf("[intcal] Calibrating output interlock ch %d\r\n", channel);
		retval = rf_channel_calibrate_output_interlock_v3(channel, dacval, 100);
		if (retval == 0) retval = 100;
		printf("[intcal] Calibration step = 100 completed = %d\n", retval);

		vTaskDelay(500);

		retval *= 1.05;
		retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 10);
		if (retval == 0) retval = 10;

		vTaskDelay(500);

		printf("[intcal] Calibration step = 10 completed = %d\n", retval);
		retval *= 1.02;
		retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 1);

		if (retval != 0) {
			printf("[intcal] Calibration step = 1 completed = %d\n", retval);

			if (pwr_type == 1) {
				int_cal_val_s = retval;
				int_cal_pwr_s = pwr_cal;

				printf("[intcal] done, got 1-point cal val %d pwr %d\r\n", retval, pwr_cal);
			} else if (pwr_type == 2) {
				int_cal_val_e = retval;
				int_cal_pwr_e = pwr_cal;

				printf("[intcal] done, got 2-point cal val %d pwr %d\r\n", retval, pwr_cal);
			}

			if (int_cal_val_s && int_cal_val_e) {
				float a = ((float)(int_cal_val_s - int_cal_val_e) / (float)(int_cal_pwr_s - int_cal_pwr_e));
				float b = ((float) int_cal_val_s - ((float)(int_cal_val_s - int_cal_val_e) / (float)(int_cal_pwr_s - int_cal_pwr_e)) * (float) int_cal_pwr_s);

//				float a = (int_cal_pwr_s - int_cal_pwr_e) / (log(int_cal_val_s) - log(int_cal_val_e));
//				float b = (int_cal_pwr_s - ((int_cal_pwr_s - int_cal_pwr_e) / (log(int_cal_val_s) - log(int_cal_val_e))) * log(int_cal_val_s));

				printf("[intcal] done, got power factors a=%0.2f b=%0.2f\n", a, b);
				ch->cal_values.hw_int_scale = a;
				ch->cal_values.hw_int_offset = b;

				if (lock_take(I2C_LOCK, portMAX_DELAY))
				{
					i2c_mux_select(channel);

					uint32_t u32_scale = 0x00;
					uint32_t u32_offset = 0x00;
					memcpy(&u32_scale, &a, sizeof(float));
					memcpy(&u32_offset, &b, sizeof(float));
					eeprom_write32(HW_INT_SCALE, u32_scale);
					eeprom_write32(HW_INT_OFFSET, u32_offset);

					int_cal_pwr_s = 0;
					int_cal_pwr_e = 0;
					int_cal_val_s = 0;
					int_cal_val_e = 0;

					lock_free(I2C_LOCK);
				}
			}
		}

		ch->cal_values.input_dac_cal_value = old_dac1;
		ch->cal_values.output_dac_cal_value = old_dac2;
	} else
		printf("[intcal] Wrong channel number\r\n");
}

static void fh_cal(void * a_data)
{
	int channel = 0;
	int type = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &type);

	channel = (uint8_t) channel;
	type = (uint8_t) type;

	uint16_t dacval = 4095;
	uint16_t retval = 0;

	if (channel < 8)
	{
		if (type == 0) {
			printf("[cal] Calibrating input interlock\n");
			retval = rf_channel_calibrate_input_interlock_v3(channel, dacval, 100);
			if (retval == 0) retval = 100;
			printf("[cal] Calibration step = 100 completed = %d\n", retval);

			vTaskDelay(500);

			retval *= 1.12;

			retval = rf_channel_calibrate_input_interlock_v3(channel, retval, 10);
			if (retval == 0) retval = 10;

			vTaskDelay(500);

			printf("[cal] Calibration step = 10 completed = %d\n", retval);
			retval *= 1.08;
			retval = rf_channel_calibrate_input_interlock_v3(channel, retval, 1);
			if (retval != 0) {
				printf("[cal] done, value = %d\n", retval);

				ch = rf_channel_get(channel);
				if (lock_take(I2C_LOCK, portMAX_DELAY)) {
					i2c_mux_select(channel);

					ch->cal_values.input_dac_cal_value = retval;
					eeprom_write16(DAC1_EEPROM_ADDRESS, retval);
					lock_free(I2C_LOCK);
				}
			} else {
				printf("[cal] error, failed\n");
			}
		}

//		if (type == 0) {
//			printf("[cal] Calibrating input interlock\n");
//			retval = rf_channel_calibrate_input_interlock(channel, dacval, 100);
//			if (retval == 0) retval = 100;
//			printf("[cal] Calibration step = 100 completed = %d\n", retval);
//
//			vTaskDelay(500);
//
//			retval /= 1.2;
//			retval = rf_channel_calibrate_input_interlock(channel, retval, 10);
//			if (retval == 0) retval = 10;
//
//			vTaskDelay(500);
//
//			printf("[cal] Calibration step = 10 completed = %d\n", retval);
//			retval /= 1.02;
//			retval = rf_channel_calibrate_input_interlock(channel, retval, 1);
//			if (retval != 0) {
//				printf("[cal] done, value = %d\n", retval);
//
//				ch = rf_channel_get(channel);
//				if (lock_take(I2C_LOCK, portMAX_DELAY)) {
//					i2c_mux_select(channel);
//
//					ch->cal_values.input_dac_cal_value = retval;
//					eeprom_write16(DAC1_EEPROM_ADDRESS, retval);
//					lock_free(I2C_LOCK);
//				}
//			} else {
//				printf("[cal] error, failed\n");
//			}
//		}

		if (type == 1) {
			printf("[cal] Calibrating output interlock\n");
			retval = rf_channel_calibrate_output_interlock_v3(channel, dacval, 100);
			if (retval == 0) retval = 100;
			printf("[cal] Calibration step = 100 completed = %d\n", retval);

			vTaskDelay(500);

			retval *= 1.1;
			retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 10);
			if (retval == 0) retval = 10;

			vTaskDelay(500);

			printf("[cal] Calibration step = 10 completed = %d\n", retval);
			retval *= 1.02;
			retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 1);
			if (retval != 0) {
				printf("[cal] done, value = %d\n", retval);

//				ch = rf_channel_get(channel);
//				if (lock_take(I2C_LOCK, portMAX_DELAY)) {
//					i2c_mux_select(channel);
//
//					ch->cal_values.output_dac_cal_value = retval;
//					eeprom_write16(DAC2_EEPROM_ADDRESS, retval);
//					lock_free(I2C_LOCK);
//				}

			} else {
				printf("[cal] Calibration failed\n");
			}
		}

//		if (type == 1) {
//				printf("[cal] Calibrating output interlock\n");
//				retval = rf_channel_calibrate_output_interlock_v3(channel, dacval, 100);
//				if (retval == 0) retval = 100;
//				printf("[cal] Calibration step = 100 completed = %d\n", retval);
//
//				vTaskDelay(500);
//
//				retval *= 1.3;
//				retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 10);
//				if (retval == 0) retval = 10;
//
//				vTaskDelay(500);
//
//				printf("[cal] Calibration step = 10 completed = %d\n", retval);
//				retval *= 1.1;
//				retval = rf_channel_calibrate_output_interlock_v3(channel, retval, 1);
//				if (retval != 0) {
//					printf("[cal] done, value = %d\n", retval);
//
//					ch = rf_channel_get(channel);
//					if (lock_take(I2C_LOCK, portMAX_DELAY)) {
//						i2c_mux_select(channel);
//
//						ch->cal_values.output_dac_cal_value = retval;
//						eeprom_write16(DAC2_EEPROM_ADDRESS, retval);
//						lock_free(I2C_LOCK);
//					}
//
//				} else {
//					printf("[cal] Calibration failed\n");
//				}
//			}
	} else
		printf("[cal] Wrong channel number\r\n");
}


static void fh_biascal(void * a_data)
{
	int channel = 0;
	int value = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &value);

	channel = (uint8_t) channel;
	value = (uint8_t) value;

	if (channel < 8)
	{
		rf_channel_calibrate_bias(channel, value);
	} else
		printf("[biascal] Wrong channel number\r\n");
}


static void fh_chanid(void * a_data)
{
	int channel = 0;
	int value = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &value);

	if ((uint8_t) channel < 8)
	{
		ch = rf_channel_get((uint8_t) channel);
		printf("[chanid] %d = %02X%02X%02X%02X%02X%02X\r\n", (uint8_t) channel, ch->hwid[0], ch->hwid[1], ch->hwid[2], ch->hwid[3], ch->hwid[4], ch->hwid[5]);
	}
	else
		printf("[chanid] Wrong channel number\r\n");
}

static void fh_i2cerr(void * a_data)
{
	printf("\t\t#0\t#1\t#2\t#3\t#4\t#5\t#6\t#7\n");
	printf("I2C ERR\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", i2c_get_channel_errors(0),
															i2c_get_channel_errors(1),
															i2c_get_channel_errors(2),
															i2c_get_channel_errors(3),
															i2c_get_channel_errors(4),
															i2c_get_channel_errors(5),
															i2c_get_channel_errors(6),
															i2c_get_channel_errors(7));
}


static void fh_int(void * a_data)
{
	int channel = 0;
	int value = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &value);

	if ((uint8_t) channel < 8) {
		ch = rf_channel_get((uint8_t) channel);
		ch->cal_values.output_dac_cal_value = (uint16_t) value;

		if (ch->enabled) {
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select((uint8_t) channel);
				i2c_dual_dac_set(1, ch->cal_values.output_dac_cal_value);
				lock_free(I2C_LOCK);
				printf("[int] Interlock value = %d\r\n", ch->cal_values.output_dac_cal_value);
			}
		}
	} else
		printf("[int] Wrong channel number\r\n");
}


static void fh_intval(void * a_data)
{
	int channel = 0;
	float value = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_float(2, &value);

	if ((uint8_t) channel < 8) {
		rf_channel_interlock_set(channel, value);
	} else
		printf("[intv] Wrong channel number\r\n");
}

static void fh_intparams(void * a_data)
{
	int channel = 0;
	float a = 0;
	float b = 0;
	channel_t *ch = NULL;

	ucli_param_get_int(1, &channel);
	ucli_param_get_float(2, &a);
	ucli_param_get_float(3, &b);

	if ((uint8_t) channel < 8) {
		ch = rf_channel_get(channel);

		ch->cal_values.hw_int_scale = a;
		ch->cal_values.hw_int_offset = b;

		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select(channel);

			uint32_t u32_scale = 0x00;
			uint32_t u32_offset = 0x00;
			memcpy(&u32_scale, &a, sizeof(float));
			memcpy(&u32_offset, &b, sizeof(float));
			eeprom_write32(HW_INT_SCALE, u32_scale);
			eeprom_write32(HW_INT_OFFSET, u32_offset);

			lock_free(I2C_LOCK);
		}

		printf("[intparams] Interlock params for ch %d, a = %0.2f, b = %0.2f\r\n", channel, a, b);
	} else
		printf("[intparams] Wrong channel number\r\n");
}

static void fh_intget(void * a_data)
{
	int channel = 0;

	ucli_param_get_int(1, &channel);

	if ((uint8_t) channel < 8) {
		double value = rf_channel_interlock_get(channel);
		printf("[intv] Interlock value for ch %d = %0.2f\r\n", channel, value);
	} else
		printf("[intv] Wrong channel number\r\n");
}

static void fh_enable(void * a_data)
{
	int channel_mask = 0;
	ucli_param_get_int(1, &channel_mask);

	rf_channels_enable((uint8_t) channel_mask);
	printf("[enable] Enabled %d channel mask\r\n", (uint8_t) channel_mask);
}


static void fh_disable(void * a_data)
{
	int channel_mask = 0;
	ucli_param_get_int(1, &channel_mask);

	rf_channels_disable((uint8_t) channel_mask);
	printf("[disable] Disabled %d channel mask\r\n", (uint8_t) channel_mask);
}


static void fh_dac(void * a_data)
{
	int channel = 0;
	int dac_channel = 0;
	int value = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &dac_channel);
	ucli_param_get_int(3, &value);

	if ((uint8_t) channel < 8) {
		if ((uint8_t) dac_channel < 3) {
			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				i2c_mux_select((uint8_t) channel);
				i2c_dual_dac_set((uint8_t) dac_channel, (uint16_t) value);
				printf("[dac] dac[%d] = %d\r\n", (uint8_t) dac_channel, (uint16_t) value);

				lock_free(I2C_LOCK);
			}
		}
	} else
		printf("[dac] Wrong channel number\r\n");
}


static void fh_clearcal(void * a_data)
{
	int channel = 0;
	int value = 0;
	channel_t * ch;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &value);

	if ((uint8_t) channel < 8)
	{
		ch = rf_channel_get((uint8_t) channel);
		ch->cal_values.input_dac_cal_value = (uint16_t) value;
		ch->cal_values.output_dac_cal_value = (uint16_t) value;

		if (lock_take(I2C_LOCK, portMAX_DELAY)) {
			i2c_mux_select((uint8_t) channel);

			eeprom_write16(DAC1_EEPROM_ADDRESS, (uint16_t) value);
			vTaskDelay(10);
			eeprom_write16(DAC2_EEPROM_ADDRESS, (uint16_t) value);
			lock_free(I2C_LOCK);
		}
		printf("[clearcal] DAC values override = %d\r\n", (uint16_t) value);
	} else
		printf("[clearcal] Wrong channel number\r\n");
}


static void fh_bias(void * a_data)
{
	int channel = 0;
	int value = 0;
	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &value);

	if ((uint8_t) channel < 8) {
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			i2c_dac_set((uint16_t) value);
			printf("[bias] DAC value = %d\r\n", (uint16_t) value);
			lock_free(I2C_LOCK);
		}
	} else
		printf("[bias] Wrong channel number\r\n");
}


static void fh_eepromr(void * a_data)
{
	int channel = 0;
	int address = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &address);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			printf("[eepromr] EEPROM reg[%d] = %d\r\n", (uint8_t) address, eeprom_read((uint8_t) address));
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[eepromr] Wrong channel number\r\n");
	}
}


static void fh_eepromw(void * a_data)
{
	int channel = 0;
	int address = 0;
	int data = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &address);
	ucli_param_get_int(3, &data);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			eeprom_write(address, (uint8_t) data);
			vTaskDelay(10);
			printf("[eepromw] EEPROM reg[%d] = %d\r\n", (uint8_t) address, eeprom_read((uint8_t) address));
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[eepromw] Wrong channel number\r\n");
	}
}


static void fh_i2cw(void * a_data)
{
	int channel = 0;
	int address = 0;
	int reg = 0;
	int data = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &address);
	ucli_param_get_int(3, &reg);
	ucli_param_get_int(4, &data);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			i2c_write(ADS7924_I2C, address, reg, data);
			printf("[i2cw] write to %d reg %d data %d\r\n", address, reg, data);
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[i2cw] Wrong channel number\r\n");
	}
}


static void fh_i2cr(void * a_data)
{
	int channel = 0;
	int address = 0;
	int reg = 0;

	ucli_param_get_int(1, &channel);
	ucli_param_get_int(2, &address);
	ucli_param_get_int(3, &reg);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			uint8_t data = 0;
			i2c_read(ADS7924_I2C, address, reg, &data);
			printf("[i2cw] read to %d reg %d = %d\r\n", address, reg, data);
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[i2cw] Wrong channel number\r\n");
	}
}


static void fh_taskstats(void * a_data)
{
	char buf[512] = { 0 };
	const char *const pcHeader = "Task\t\tState\tPriority\tStack\t#\r\n**************************************************\r\n";

	strcpy( buf, pcHeader );
	vTaskList( buf + strlen( pcHeader ) );
	puts(buf);
}


static void fh_netconfig(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	uint8_t ipaddr[15] = { 0 };
	uint8_t ipdst[15] = { 0 };
	uint8_t subnet[15] = { 0 };

	if (a_ctx->argc == 1) {
		display_net_conf();
	} else if (a_ctx->argc == 2) {
		if (!strcmp(a_ctx->argv[1], "dhcp")) {
			prvDHCPTaskRestart();
			printf("[ifconfig] dhcp\r\n");
		}
	} else if (a_ctx->argc == 4) {
		uint8_t check = 1;

		if (prvCheckValidIPAddress(a_ctx->argv[1], ipaddr) != 4) check = 0;
		if (prvCheckValidIPAddress(a_ctx->argv[2], ipdst) != 4) check = 0;
		if (prvCheckValidIPAddress(a_ctx->argv[3], subnet) != 4) check = 0;

		if (check)
		{
			set_net_conf(ipaddr, ipdst, subnet);
			printf("[ifconfig] static %s %s %s\r\n", ipaddr, ipdst, subnet);
		} else
			printf("[ifconfig] error while parsing parameters\r\n");
	}
}


static void fh_macconfig(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	uint8_t check = 1;
	uint8_t macdata[6];

	if (!strcmp(a_ctx->argv[1], "default"))
	{
		set_default_mac_address();
		printf("[macconfig] Default MAC address set\r\n");
		return;
	}

	if (prvCheckValidMACAddress(a_ctx->argv[1], macdata) != 6) check = 0;

	if (check) {
		printf("[macconfig] MAC address %02X:%02X:%02X:%02X:%02X:%02X set\r\n", macdata[0],
																				macdata[1],
																				macdata[2],
																				macdata[3],
																				macdata[4],
																				macdata[5]);
		set_mac_address(macdata);
	} else
		printf("[macconfig] Wrong MAC address specified\r\n");
}

static void fh_ethdbg(void * a_data)
{
	if (lock_take(ETH_LOCK, portMAX_DELAY))
	{
		printf("getSn_SR %d\r\n", getSn_SR(0));
		printf("getSn_MR %d\r\n", getSn_MR(0));
		printf("getSn_RX_RSR %d\r\n", getSn_RX_RSR(0));
		printf("getSn_CR %d\r\n", getSn_CR(0));
		printf("getSn_IR %d\r\n", getSn_IR(0));
		printf("getSn_RXBUF_SIZE %d\r\n", getSn_RXBUF_SIZE(0));

		lock_free(ETH_LOCK);
	} else {
		printf("[ethdbg] Failed to acquire Ethernet lock\r\n");
	}
}

static void fh_wdtest(void * a_data)
{
	printf("Waiting for watchdog reset\r\n");
	for (;;);
}

void cli_init(void)
{
	ucli_init((void*) __io_putchar, g_cmds);
}

void vCommandConsoleTask(void *pvParameters)
{
	char ch;

	for (;;)
	{
		if (xQueueReceive(_xRxQueue, &ch, portMAX_DELAY))
		{
			ucli_process_chr(ch);
		}
	}
}


static ucli_cmd_t g_cmds[] = {
	{ "devid", fh_devid, 0x00, "Print device unique id\r\n" },
	{ "reboot", fh_reboot, 0x00, "Reboot device\r\n" },
	{ "version", fh_sw_version, 0x00, "Print software version information\r\n" },
	{ "bootloader", fh_bootloader, 0x00, "Enter DFU bootloader\r\n" },
	{ "eepromr", fh_eepromr, 0x02, "Read module EEPROM address\r\n", "eepromr usage:\r\n\teepromr <channel id> <address>\r\n\tUsed to read selected channel module EEPROM memory.\r\n\t> eepromr 5 0\r\n\t[eepromr] EEPROM reg[0] = 2\r\n" },
	{ "eepromw", fh_eepromw, 0x03, "Write module EEPROM address\r\n", "eepromw usage:\r\n\teepromw <channel id> <address> <data>\r\n\tUsed to write to selected channel module EEPROM memory.\r\n\t> eepromw 5 0 2\r\n\t[eepromw] EEPROM reg[0] = 2\r\n" },
	{ "task-stats", fh_taskstats, 0x00, "Display task info\r\n" },
	{ "start", fh_start, 0x00, "Start the watch thread\r\n" },
	{ "stop", fh_stop, 0x00, "Stop the watch thread\r\n" },
	{ "ifconfig", fh_netconfig, -1, "Change or display network settings\r\n", "ifconfig usage:\r\n\tifconfig - display network configuration info\r\n\tifconfig dhcp - enable DHCP\r\n\tifconfig <ip src> <gw ip> <netmask> - set static network address\r\n" },
	{ "macconfig", fh_macconfig, 0x01, "Change network MAC address\r\n", "macconfig usage:\r\n\tmacconfig default - restore original MAC address\r\n\tmacconfig XX:XX:XX:XX:XX:XX - sets selected MAC address\r\n" },
	{ "enable", fh_enable, 0x01, "Enable selected channel mask\r\n", "enable usage:\r\n\tenable <channel mask> - enable specified channel with bitmask. eg 255 enables all channels = 11111111\r\n" },
	{ "disable", fh_disable, 0x01, "Disable selected channel mask\r\n", "disable usage:\r\n\tdisable <channel mask> - disable specified channel with bitmask. eg 255 disables all channels = 11111111\r\n" },
	{ "bias", fh_bias, 0x02, "Set channel bias voltage DAC value\r\n" , "bias usage:\r\n\tbias <channel> <value> - sets specified DAC value for selected channel in order to adjust bias current. Changes are not saved to EEPROM" },
	{ "int", fh_int, 0x02, "Set output interlock by raw value\r\n", "int usage:\r\n\tint <channel> <value> - sets raw value to output interlock comparator\r\n" },
	{ "intv", fh_intval, 0x02, "Set output interlock by power level\r\n", "intv usage:\r\n\tintv <channel> <power> - sets value for output interlock comparator based on output power\r\n" },
	{ "chanid", fh_chanid, 0x01, "Display channel hwid\r\n" },
	{ "dac", fh_dac, 0x03, "Set module DAC channel values\r\n", "dac usage:\r\n\tdac <channel> <dac_channel> <value> - sets selected channel DAC values. DAC channel range 0-1, value 0-65535\r\n" },
	{ "detected", fh_detected, 0x00, "Return detected channel bit mask\r\n" },
	{ "enabled", fh_enabled, 0x00, "Show enabled and sigon channel masks\r\n" },
	{ "status", fh_status, 0x01, "Display channel brief status\r\n" },
	{ "i2cdetect", fh_i2cd, 0x01, "Detect I2C devices on selected channel\r\n", "i2cdetect usage:\r\n\ti2cdetect <channel> - scan I2C bus for selected channel\r\n"},
	{ "clearint", fh_clearint, -1, "Clear interlock status of selected channel\r\n", "clearcal usage:\r\n\tclearcal <channel> - clear selected channel interlock. If channel number is left blank all channel interlocks are cleared\r\n" },
	{ "i2cw", fh_i2cw, 0x04, "Write I2C on selected channel\r\n", "i2cw usage:\r\n\ti2cw <channel> <address> <data> - write one byte to selected channel address\r\n" },
	{ "i2cr", fh_i2cr, 0x03, "Read I2C on selected channel\r\n", "i2cr usage:\r\n\ti2cw <channel> <address> - read one byte from selected channel address\r\n" },
	{ "currents", fh_currents, 0x00, "Return list of all P30V currents\r\n"},
	{ "powercfg", fh_powercfg, -1, "Configure powering channels after boot\r\n"},
	{ "fanlevel", fh_fanlevel, -1, "Configure minimum fan level while channels are enabled\r\n"},

	// "hidden" commands not for end-user
	{ "wdtest", fh_wdtest, 0x00 },
	{ "calpwr", fh_calpwr, 0x03 },
	{ "intcal", fh_intcal, 0x03 },
	{ "intg", fh_intget, 0x01 },
	{ "intparams", fh_intparams, 0x03 },
	{ "cal", fh_cal, 0x02 },
	{ "biascal", fh_biascal, 0x02 },
	{ "clearcal", fh_clearcal, 0x02 },
	{ "i2cerr", fh_i2cerr, 0x00 },
	{ "ethdbg", fh_ethdbg, 0x00 },

    // null
    { 0x00, 0x00, 0x00  }
};
