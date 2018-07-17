/*
 * server.c
 *
 *  Created on: Aug 1, 2017
 *      Author: wizath
 */

#include "wizchip_conf.h"
#include "socket.h"
#include "server.h"
#include "locks.h"

#include "scpi/scpi.h"
extern scpi_t scpi_context;

#define MAX_RX_LENGTH 1024

SemaphoreHandle_t xUDPMessageAvailable;

static void udp_int_init(void)
{
	GPIO_InitTypeDef 	GPIO_InitStructure;
	EXTI_InitTypeDef 	EXTI_InitStruct;
	NVIC_InitTypeDef 	NVIC_InitStruct;

	// interrupt PG6
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

	GPIO_Init(GPIOG, &GPIO_InitStructure);
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOG, EXTI_PinSource6);

	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x0F;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x0F;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void EXTI9_5_IRQHandler(void)
{
	static long xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;

	if (EXTI_GetITStatus(EXTI_Line6) != RESET)
	{
		GPIO_ToggleBits(BOARD_LED1);
		xSemaphoreGiveFromISR(xUDPMessageAvailable, &xHigherPriorityTaskWoken);
    	EXTI_ClearITPendingBit(EXTI_Line6);
    	NVIC_DisableIRQ(EXTI9_5_IRQn);
    }

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void prvSetupUDPServer(void)
{
	// init RECV semaphore
	vSemaphoreCreateBinary(xUDPMessageAvailable);
	xSemaphoreTake(xUDPMessageAvailable, 0);

	// create sockets
	socket(0, Sn_MR_UDP, 5000, 0);
	setSn_IMR(0, 4); // set RECV interrupt mask

	if(getSn_SR(0) == SOCK_UDP)
	{
		printf("[log] UDP Server initialization complete\n");
	}
}

void prvUDPServerTask(void *pvParameters)
{
	uint32_t ir;
	int32_t  len;
	uint8_t  destip[4];
	uint16_t destport;
	uint8_t rx_buffer[MAX_RX_LENGTH];

	printf("[log] network init done, starting server..\n");
	udp_int_init();
	prvSetupUDPServer();

	uint16_t imr = IK_SOCK_0;
	if (ctlwizchip(CW_SET_INTRMASK, &imr) == -1) {
		printf("[dbg] cannot set imr");
	}

	for (;;)
	{
		if (xSemaphoreTake(xUDPMessageAvailable, configTICK_RATE_HZ))
		{
			if (lock_take(ETH_LOCK, portMAX_DELAY)) {
				ctlwizchip(CW_GET_INTERRUPT, &ir);
				if (ir & IK_SOCK_0)
				{
					uint8_t sir = getSn_IR(0);
					if ((sir & Sn_IR_RECV) > 0)
					{
						len = getSn_RX_RSR(0);
						if (len > MAX_RX_LENGTH) len = MAX_RX_LENGTH;
						len = recvfrom(0, rx_buffer, len, destip, (uint16_t*)&destport);
//						rx_buffer[len] = '\0';
//						printf("%d | %s\n", (int) len, rx_buffer);

						if (scpi_context.user_context != NULL) {
							user_data_t * u = (user_data_t *) (scpi_context.user_context);
							memcpy(u->ipsrc, destip, 4);
							u->ipsrc_port = destport;
						}

						SCPI_Input(&scpi_context, (char *) rx_buffer, (int) len);

//						sendto(0, rx_buffer, len, destip, destport);
						setSn_IR(0, Sn_IR_RECV);
					}

					setSn_IR(0, Sn_IR_RECV);
					ctlwizchip(CW_CLR_INTERRUPT, (void*) IK_SOCK_0);
					NVIC_EnableIRQ(EXTI9_5_IRQn);
				}

				lock_free(ETH_LOCK);
			}
		}
	}
}
