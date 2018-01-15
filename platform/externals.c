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

QueueHandle_t ExtQueue;

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

	ExtQueue = xQueueCreate(8, sizeof( uint8_t ));
	xQueueReset(ExtQueue);

	ext_timer_init();
}

void ext_timer_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/* TIM4 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/* Enable the TIM4 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock) / 100) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = 20 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM4, PrescalerValue, TIM_PSCReloadMode_Immediate);

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
}

void TIM4_IRQHandler()
{
	static long xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	static uint8_t count_btn1 = 0;
	static uint8_t state_btn1 = 1;

	static uint8_t count_btn2 = 0;
	static uint8_t state_btn2 = 1;

	uint8_t evt_id = 0;

    if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
    {
    	uint8_t btn1 = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_14);
		if (btn1 != state_btn1) {
			count_btn1++;
			if (count_btn1 >= 4) {
				state_btn1 = btn1;
				if (state_btn1 != 0) {
					evt_id = 1;
					xQueueSendFromISR(ExtQueue, &evt_id, xHigherPriorityTaskWoken);
				}
			}
		} else count_btn1 = 0;

		uint8_t btn2 = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_15);
		if (btn2 != state_btn2) {
			count_btn2++;
			if (count_btn2 >= 4) {
				state_btn2 = btn2;
				if (state_btn2 != 0) {
					evt_id = 2;
					xQueueSendFromISR(ExtQueue, &evt_id, xHigherPriorityTaskWoken);
				}
			}
		} else count_btn2 = 0;

        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void prvExtTask(void *pvParameters)
{
	uint8_t btn_evt = 0;
	bool onstate = 0;

	for (;;)
	{
		if (xQueueReceive(ExtQueue, &btn_evt, portMAX_DELAY))
		{
			switch (btn_evt)
			{
				case 1:
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
					break;
				case 2:
					// quick fix: enable SIGON only when there is power
					// on ch2
					if (GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_1) && GPIO_ReadOutputDataBit(GPIOD, GPIO_Pin_0))
					{
						led_bar_write(0, 0, 0);
						rf_channels_sigon(0xFF, false);
						vTaskDelay(configTICK_RATE_HZ / 10);

						rf_sigon_enable(CHANNEL_MASK);
						led_bar_write(rf_channels_read_sigon(), 0, 0);
					}
					break;
			}

			vTaskDelay(10);
		}
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
