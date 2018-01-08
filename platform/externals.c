/*
 * externals.c
 *
 *  Created on: Dec 15, 2017
 *      Author: wizath
 */

#include "config.h"
#include "stm32f4xx_exti.h"
#include "stm32f4xx_gpio.h"
#include "led_bar.h"
#include "channels.h"

#define CHANNEL_MASK 0b00000011

void ext_init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	EXTI_InitTypeDef 	EXTI_InitStruct;
	NVIC_InitTypeDef 	NVIC_InitStruct;

	// BUTTONS
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOF, &GPIO_InitStructure);

	// PGOOD
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOF, &GPIO_InitStructure);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource4);

	EXTI_InitStruct.EXTI_Line = EXTI_Line4;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = EXTI4_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void prvExtTask(void *pvParameters)
{
	uint8_t btnp = 0, btn = 0;
	uint8_t btnp2 = 0, btn2 = 0;
	uint8_t onstate = 0, onstate2 = 0;

	for (;;)
	{
		btn = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_14);
		if (btnp != btn)
		{
			btnp = btn;
			if (!btn) {
				if (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4))
				{
					onstate = !onstate;
					if (onstate) {
						rf_channels_enable(CHANNEL_MASK);
					} else {
						rf_disable_dac();
						rf_channels_control(255, false);
						rf_channels_sigon(255, false);
						led_bar_write(0, 0, 0);
					}
				}
			}
		}

		btn2 = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_15);
		if (btnp2 != btn2)
		{
			btnp2 = btn2;
			if (!btn2) {
				onstate2 = !onstate2;
				if (onstate2)
				{
					// quick fix: enable SIGON only when there is power
					// on ch2
					if (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_1))
					{
						led_bar_write(0, 0, 0);
						rf_channels_sigon(0xFF, false);
						vTaskDelay(configTICK_RATE_HZ / 10);
						rf_sigon_enable(CHANNEL_MASK);
						led_bar_write(rf_channels_read_sigon(), 0, 0);
					}
				}
			}
		}

		vTaskDelay(50);
	}
}

void EXTI4_IRQHandler(void)
{
	static long xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	if (EXTI_GetITStatus(EXTI_Line4) != RESET)
	{
		rf_disable_dac();
		rf_channels_control(255, false);
		led_bar_write(0, 0, 255);
    	EXTI_ClearITPendingBit(EXTI_Line4);
    }

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
