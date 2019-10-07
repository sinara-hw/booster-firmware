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

	float input_pwr_scale;
	float input_pwr_offset;

	float hw_int_scale;
	float hw_int_offset;
} channel_cal_t;

typedef struct {
	/* Power measurements */
	uint16_t adc_raw_ch1;
	uint16_t adc_raw_ch2;
	double   adc_ch1;
	double   adc_ch2;
	double   adc_pwr_ch1;
	double   adc_pwr_ch2;

	/* input power measurement */
	uint16_t input_raw;
	double input_voltage;
	double input_power;

	/* Current measurements */
	double i30;
	double i60;
	double p5v0mp;

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
	bool error;

	bool input_interlock;
	bool output_interlock;
	bool soft_interlock;
	bool overcurrent;

	double interlock_setpoint;

	channel_cal_t cal_values;
	channel_meas_t measure;
	uint8_t hwid[6];
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
bool rf_channel_clear_interlock(uint8_t channel);
uint8_t rf_channels_get_mask(void);
void rf_channels_soft_interlock_set(uint8_t channel, double value);
void rf_channels_hwint_override(uint8_t channel, double int_value);
void rf_channel_disable_on_error(uint8_t channel);

uint16_t rf_channel_interlock_set(uint8_t channel, double value);
double rf_channel_interlock_get(uint8_t channel);

/* tasks */
void rf_channels_interlock_task(void *pvParameters);
void rf_channels_measure_task(void *pvParameters);
void rf_channels_info_task(void *pvParameters);

/* calibration tasks */
uint16_t rf_channel_calibrate_input_interlock_v3(uint8_t channel, int16_t start_value, uint8_t step);
uint16_t rf_channel_calibrate_output_interlock_v3(uint8_t channel, int16_t start_value, uint8_t step);
bool rf_channel_calibrate_bias(uint8_t channel, uint16_t current);
void rf_channel_disable_on_i2c_err(uint8_t channel);


#endif /* CHANNELS_H_ */

