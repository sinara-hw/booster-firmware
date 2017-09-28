/*
 * config.h
 *
 *  Created on: Sep 12, 2017
 *      Author: rst
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#define BOARD_LED1 GPIOC, GPIO_Pin_9
#define BOARD_LED2 GPIOC, GPIO_Pin_8
#define BOARD_LED3 GPIOC, GPIO_Pin_10

#define BOARD_ADC_IN0 		GPIOA, GPIO_Pin_0
#define BOARD_ADC_IN1 		GPIOA, GPIO_Pin_1
#define BOARD_ADC_IN2 		GPIOA, GPIO_Pin_2
#define BOARD_ADC_IN3 		GPIOA, GPIO_Pin_3
#define BOARD_ADC_IN4 		GPIOF, GPIO_Pin_6
#define BOARD_ADC_IN5 		GPIOF, GPIO_Pin_7
#define BOARD_ADC_IN6 		GPIOF, GPIO_Pin_8
#define BOARD_ADC_IN7 		GPIOF, GPIO_Pin_9
#define BOARD_ADC_IN8 		GPIOF, GPIO_Pin_10
#define BOARD_ADC_IN9 		GPIOF, GPIO_Pin_3
#define BOARD_ADC_IN10 		GPIOC, GPIO_Pin_0
#define BOARD_ADC_IN11 		GPIOC, GPIO_Pin_1
#define BOARD_ADC_IN12 		GPIOC, GPIO_Pin_2
#define BOARD_ADC_IN13 		GPIOC, GPIO_Pin_3
#define BOARD_ADC_IN14 		GPIOF, GPIO_Pin_4
#define BOARD_ADC_IN15 		GPIOF, GPIO_Pin_5

#define BOARD_WIZNET_CS 	GPIOA, GPIO_Pin_4
#define BOARD_WIZNET_RST	GPIOG, GPIO_Pin_5
#define BOARD_WIZNET_INT	GPIOG, GPIO_Pin_6


#endif /* CONFIG_H_ */
