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
#include "stdbool.h"

#include "stm32f4xx.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#define BOARD_LED1 			GPIOC, GPIO_Pin_9
#define BOARD_LED2 			GPIOC, GPIO_Pin_8
#define BOARD_LED3 			GPIOC, GPIO_Pin_10

#define BOARD_WIZNET_CS 	GPIOA, GPIO_Pin_4
#define BOARD_WIZNET_RST	GPIOG, GPIO_Pin_5
#define BOARD_WIZNET_INT	GPIOG, GPIO_Pin_6

#define SPACER " "

#define BOARD_MANUFACTURER "WUT"
#define BOARD_NAME		   "RFPA Booster"
#define BOARD_REV		   "1.1"
#define BOARD_YEAR		   __DATE__ SPACER __TIME__

#define CHANNEL_MASK 		0b11111111

#define PGOOD				GPIOG, GPIO_Pin_4

#endif /* CONFIG_H_ */
