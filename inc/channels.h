/*
 * channels.h
 *
 *  Created on: Oct 2, 2017
 *      Author: wizath
 */

#ifndef CHANNELS_H_
#define CHANNELS_H_

#include "config.h"

#define CH0_ENABLE 		GPIOD, GPIO_Pin_0
#define CH1_ENABLE 		GPIOD, GPIO_Pin_1
#define CH2_ENABLE 		GPIOD, GPIO_Pin_2
#define CH3_ENABLE 		GPIOD, GPIO_Pin_3
#define CH4_ENABLE 		GPIOD, GPIO_Pin_4
#define CH5_ENABLE 		GPIOD, GPIO_Pin_5
#define CH6_ENABLE 		GPIOD, GPIO_Pin_6
#define CH7_ENABLE 		GPIOD, GPIO_Pin_7

#define CH0_ALERT 		GPIOD, GPIO_Pin_8
#define CH1_ALERT 		GPIOD, GPIO_Pin_9
#define CH2_ALERT 		GPIOD, GPIO_Pin_10
#define CH3_ALERT 		GPIOD, GPIO_Pin_11
#define CH4_ALERT 		GPIOD, GPIO_Pin_12
#define CH5_ALERT 		GPIOD, GPIO_Pin_13
#define CH6_ALERT 		GPIOD, GPIO_Pin_14
#define CH7_ALERT 		GPIOD, GPIO_Pin_15

#define CH0_USERIO 		GPIOE, GPIO_Pin_0
#define CH1_USERIO 		GPIOE, GPIO_Pin_1
#define CH2_USERIO 		GPIOE, GPIO_Pin_2
#define CH3_USERIO 		GPIOE, GPIO_Pin_3
#define CH4_USERIO 		GPIOE, GPIO_Pin_4
#define CH5_USERIO 		GPIOE, GPIO_Pin_5
#define CH6_USERIO 		GPIOE, GPIO_Pin_6
#define CH7_USERIO 		GPIOE, GPIO_Pin_7

#define CH0_OVL 		GPIOE, GPIO_Pin_8
#define CH1_OVL 		GPIOE, GPIO_Pin_9
#define CH2_OVL 		GPIOE, GPIO_Pin_10
#define CH3_OVL 		GPIOE, GPIO_Pin_11
#define CH4_OVL 		GPIOE, GPIO_Pin_12
#define CH5_OVL 		GPIOE, GPIO_Pin_13
#define CH6_OVL 		GPIOE, GPIO_Pin_14
#define CH7_OVL 		GPIOE, GPIO_Pin_15

#define CH0_SIGON 		GPIOG, GPIO_Pin_8
#define CH1_SIGON 		GPIOG, GPIO_Pin_9
#define CH2_SIGON 		GPIOG, GPIO_Pin_10
#define CH3_SIGON 		GPIOG, GPIO_Pin_11
#define CH4_SIGON 		GPIOG, GPIO_Pin_12
#define CH5_SIGON 		GPIOG, GPIO_Pin_13
#define CH6_SIGON 		GPIOG, GPIO_Pin_14
#define CH7_SIGON 		GPIOG, GPIO_Pin_15

typedef struct {
	bool enabled;
	bool overvoltage;
	bool alert;
	bool userio;
	bool sigon;

	uint16_t adc_raw_ch1;
	uint16_t adc_raw_ch2;
	double   adc_ch1;
	double   adc_ch2;

	uint16_t pwr_ch1;
	uint16_t pwr_ch2;
	uint16_t pwr_ch3;
	uint16_t pwr_ch4;
} channel_t;

void rf_channels_init(void);
void rf_channels_control(uint16_t channel_mask, bool enable);
uint8_t rf_channels_read_alert(void);
uint8_t rf_channels_read_ovl(void);
void rf_channels_sigon(uint16_t channel_mask, bool enable);
void rf_sigon_enable(uint8_t mask);
uint8_t rf_available_channels(uint8_t * arr);
void prcRFChannelsTask(void *pvParameters);
uint8_t rf_channels_read_enabled(void);
uint8_t rf_channels_read_user(void);
bool rf_channel_enable_procedure(uint8_t channel);
void rf_channels_enable(uint8_t mask);
void rf_disable_dac(void);
uint8_t rf_channels_read_sigon(void);

#endif /* CHANNELS_H_ */
