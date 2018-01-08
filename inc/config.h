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

#define BOARD_MANUFACTURER "ISEPW"
#define BOARD_NAME		   "RFPA"
#define BOARD_REV		   "1.0"
#define BOARD_YEAR		   "10-17"

#define PGOOD				GPIOG, GPIO_Pin_4

#endif /* CONFIG_H_ */
