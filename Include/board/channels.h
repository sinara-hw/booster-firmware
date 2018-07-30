/*
 * channels.h
 *
 *  Created on: Oct 2, 2017
 *      Author: wizath
 */

#ifndef CHANNELS_H_
#define CHANNELS_H_

#include "config.h"

#define CHANNEL_COUNT		8

typedef struct {
	uint16_t input_dac_cal_value;
	uint16_t output_dac_cal_value;
	uint16_t bias_dac_cal_value;

	uint16_t fwd_pwr_offset;
	uint16_t fwd_pwr_scale;

	uint16_t rfl_pwr_offset;
	uint16_t rfl_pwr_scale;

} channel_cal_t;

typedef struct {
	/* Power measurements */
	uint16_t adc_raw_ch1;
	uint16_t adc_raw_ch2;
	double   adc_ch1;
	double   adc_ch2;
	double   adc_pwr_ch1;
	double   adc_pwr_ch2;

	/* Current measurements */
	double i30;
	double i60;
	double in80;

	/* Temperatures */
	double local_temp;
	double remote_temp;

	double fwd_pwr;
	double rfl_pwr;
} channel_meas_t;

typedef struct {
	bool detected; // channel detected
	bool enabled;  // channel enabled
	bool sigon;	   // rf switch enabled

	bool input_interlock;
	bool output_interlock;
	bool soft_interlock;
	bool overcurrent;

	uint16_t soft_interlock_value;
	bool soft_interlock_enabled;

	channel_cal_t cal_values;
	channel_meas_t measure;
} channel_t;

/* Initialization functions */
void rf_channels_init(void);
uint8_t rf_channels_detect(void);
void rf_channel_load_values(channel_t * ch);

/* GPIO Control functions */
void rf_channels_control(uint16_t channel_mask, bool enable);
void rf_channels_sigon(uint16_t channel_mask, bool enable);
uint8_t rf_channels_read_alert(void);
uint8_t rf_channels_read_user(void);
uint8_t rf_channels_read_sigon(void);
uint8_t rf_channels_read_ovl(void);
uint8_t rf_channels_read_enabled(void);

/* Control procedures */
void rf_channels_enable(uint8_t mask);
bool rf_channel_enable_procedure(uint8_t channel);
void rf_channels_disable(uint8_t mask);
void rf_channel_disable_procedure(uint8_t channel);
void rf_sigon_enable(uint8_t mask);

/* misc */
channel_t * rf_channel_get(uint8_t num);
void rf_clear_interlock(void);
uint8_t rf_channels_get_mask(void);

/* tasks */
void rf_channels_interlock_task(void *pvParameters);
void rf_channels_measure_task(void *pvParameters);
void rf_channels_info_task(void *pvParameters);

/* calibration tasks */
uint16_t rf_channel_calibrate_input_interlock(uint8_t channel, int16_t start_value, uint8_t step);
uint16_t rf_channel_calibrate_output_interlock(uint8_t channel, int16_t start_value, uint8_t step);
bool rf_channel_calibrate_bias(uint8_t channel, uint16_t current);


#endif /* CHANNELS_H_ */

