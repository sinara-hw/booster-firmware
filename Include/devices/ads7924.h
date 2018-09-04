/*
 * ads7924.h
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#ifndef ADS7924_H_
#define ADS7924_H_

#include "config.h"

#define ADS7924_ADDRESS 		0x49
#define ADS7924_I2C				I2C1

#define MODECNTRL_REG			0x00
#define INTCNTRL_REG			0x01
#define DATA0_REG				0x02
#define DATA1_REG				0x04
#define DATA2_REG				0x06
#define DATA3_REG				0x08
#define UT0_REG					0x0A
#define LT0_REG					0x0B
#define UT1_REG					0x0C
#define LT1_REG					0x0D
#define UT2_REG					0x0E
#define LT2_REG					0x0F
#define UT3_REG					0x10
#define LT3_REG					0x11
#define INTCONFIG_REG			0x12
#define SLPCONFIG_REG			0x13
#define ACQCONFIG_REG			0x14
#define PWRCONFIG_REG			0x15
#define RESET_REG				0x16

#define MODE_IDLE 				0
#define MODE_AWAKE				1
#define MODE_MANUAL_SINGLE 		2
#define MODE_MANUAL_SCAN		3
#define MODE_AUTO_SINGLE		4
#define MODE_AUTO_SCAN			5
#define MODE_AUTO_SINGLE_SLEEP	6
#define MODE_AUTO_SCAN_SLEEP	7
#define MODE_AUTO_BURST_SLEEP	8

#define RESET_CMD				0b10101010

void ads7924_reset(void);
void ads7924_set_threshholds(uint8_t ch, uint8_t upper, uint8_t lower);
void ads7924_get_threshholds(uint8_t ch, uint8_t * upper, uint8_t * lower);
void ads7924_set_mode(uint8_t mode);
void ads7924_set_alarm(uint8_t ch);
uint8_t ads7924_get_alarm(void);
uint16_t ads7924_get_channel_data(uint8_t ch);
double ads7924_get_channel_voltage(uint8_t ch);
void ads7924_get_data(uint16_t * data);
void ads7924_init(void);
void ads7924_stop(void);

/* threshold alerts for ch0 */
void ads7924_enable_alert(void);
void ads7924_disable_alert(void);
uint8_t ads7924_clear_alert(void);

#endif /* ADS7924_H_ */
