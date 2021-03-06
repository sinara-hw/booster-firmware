/*
 * server.c
 *
 *  Created on: Aug 1, 2017
 *      Author: wizath
 */

#include "math.h"
#include "wizchip_conf.h"
#include "socket.h"
#include "server.h"
#include "locks.h"
#include "ucli.h"

#include "scpi/scpi.h"
extern scpi_t scpi_context;

#define MAX_RX_LENGTH 640

xQueueHandle xTCPServerIRQ;

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
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x01;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x01;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStruct);
}

void EXTI9_5_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	uint8_t trg = 0x01;

	if (EXTI_GetITStatus(EXTI_Line6) != RESET)
	{
//		GPIO_ToggleBits(BOARD_LED1);
		xQueueSendFromISR(xTCPServerIRQ, &trg, &xHigherPriorityTaskWoken);
    	EXTI_ClearITPendingBit(EXTI_Line6);
    }

	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void prvSetupUDPServer(void)
{
	// init RECV queue
	xTCPServerIRQ = xQueueCreate(32, sizeof(uint8_t));

	uint8_t result = 0;
	uint8_t state = 0;

	for (int i = 0; i < 4; i++)
	{
		uint8_t ret = socket(i, Sn_MR_TCP, 5000, 0x00);
		result |= ( ret == i ? 0 : 1 ) << i; 	// set 1 if error occurred
		setSn_IMR(i, 15); 						// set RECV interrupt mask

		uint8_t sock_state = getSn_SR(0);
		result |= ( sock_state == SOCK_INIT ? 0 : 1 ) << i;
	}

	if (result != 0)
		ucli_log(UCLI_LOG_ERROR, "TCP Server initialization fail, status %d\r\n", result);
	else
		ucli_log(UCLI_LOG_INFO, "TCP Server initialization complete\r\n");

	ucli_log(UCLI_LOG_INFO, "network socket state %s\r\n", !state ? "OK" : "ERR");
}

static void process_server_data(void)
{
	uint32_t 	ir;
	int32_t  	len;
	uint8_t  	destip[4];
	uint16_t 	destport;
	uint8_t 	rx_buffer[MAX_RX_LENGTH];

	uint8_t sn = 0;
	uint8_t last_sn = 0;

	// we can feed the dog inside to avoid interrupts
	// because CPU get's really busy with VCP and SCPI
	IWDG_ReloadCounter();

	if (lock_take(ETH_LOCK, portMAX_DELAY)) {

		ctlwizchip(CW_GET_INTERRUPT, &ir);

		if (ir & IK_SOCK_0) {
			sn = 0;
		} else if (ir & IK_SOCK_1) {
			sn = 1;
		} else if (ir & IK_SOCK_2) {
			sn = 2;
		} else if (ir & IK_SOCK_3) {
			sn = 3;
		}

		// avoid handling only one socket continously
		// if there are other waiting
		if (last_sn == sn)
		{
			uint32_t sn_ir = 256 * (1 << last_sn);
			uint32_t ir_mask = (ir & ~(sn_ir)); // exclude last_sn from ir list

			if (ir_mask != 0) {
				if (ir_mask & IK_SOCK_0) {
					sn = 0;
				} else if (ir_mask & IK_SOCK_1) {
					sn = 1;
				} else if (ir_mask & IK_SOCK_2) {
					sn = 2;
				} else if (ir_mask & IK_SOCK_3) {
					sn = 3;
				}
			}
		}

		switch (getSn_SR(sn))
		{
			case SOCK_ESTABLISHED:
				if (getSn_IR(sn) & Sn_IR_CON)
				{
					getSn_DIPR(sn, destip);
					destport = getSn_DPORT(sn);
					setSn_IR(sn, Sn_IR_CON);
					ucli_log(UCLI_LOG_INFO, "network client %d.%d.%d.%d connected\r\n", destip[0], destip[1], destip[2], destip[3]);

					// set user context ip address and port
					if (scpi_context.user_context != NULL) {
						user_data_t * u = (user_data_t *) (scpi_context.user_context);
						u->socket = sn;
						memcpy(u->ipsrc[sn], destip, 4);
						u->ipsrc_port[sn] = destport;
					}
				}

				GPIO_SetBits(BOARD_LED2);
				len = getSn_RX_RSR(sn);
				if (len >= MAX_RX_LENGTH) len = MAX_RX_LENGTH-1;

				recv(sn, rx_buffer, len);
				rx_buffer[len] = '\0';

				if (scpi_context.user_context != NULL) {
					user_data_t * u = (user_data_t *) (scpi_context.user_context);
					u->socket = sn;
				}

//				ucli_log(UCLI_LOG_DEBUG, "network debug received %s\r\n", rx_buffer);
				SCPI_Input(&scpi_context, (char *) rx_buffer, (int) len);
				GPIO_ResetBits(BOARD_LED2);
				break;
			case SOCK_CLOSE_WAIT:
			case SOCK_CLOSED:
				ucli_log(UCLI_LOG_INFO, "network client disconnected\r\n");
				disconnect(sn);
				socket(sn, Sn_MR_TCP, 5000, 0x00);
				listen(sn);
				break;
			case SOCK_LISTEN:
				break;
			default:
				ucli_log(UCLI_LOG_ERROR, "Unhandled network socket event %d\r\n", getSn_SR(sn));
				break;
		}

		// clear socket interrupt
		setSn_IR(sn, 15);

		if (sn == 0)
			ctlwizchip(CW_CLR_INTERRUPT, (void*) IK_SOCK_0);
		else if (sn == 1)
			ctlwizchip(CW_CLR_INTERRUPT, (void*) IK_SOCK_1);
		else if (sn == 2)
			ctlwizchip(CW_CLR_INTERRUPT, (void*) IK_SOCK_2);
		else if (sn == 3)
			ctlwizchip(CW_CLR_INTERRUPT, (void*) IK_SOCK_3);

		last_sn = sn;

		// delay is necessary since W5500 can lost an interrupt
		// while being supressed by large number of packets
		// slowing down connection a little bit helps
		// to maintain stable response time
		vTaskDelay(5);

		// WIZNET IRQ GLITCH WORKAROUND
		if (!GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_6))
		{
			// since w5500 irq is level based
			// we issue another queue item if the interrupt
			// is low after clearing because that indicates that
			// another action is in progress and we need to process that
			uint8_t trg = 0x01;
			xQueueSend( xTCPServerIRQ, ( void * ) &trg, ( TickType_t ) 0);
		}

		lock_free(ETH_LOCK);
	}
}

void prvUDPServerTask(void *pvParameters)
{
	uint8_t 	trg = 0x00;

	/* PHY link status check */
	uint8_t tmp = 0;
	do
	{
		if (lock_take(ETH_LOCK, portMAX_DELAY))
		{
			if(ctlwizchip(CW_GET_PHYLINK, (void*) &tmp) == -1) {
				ucli_log(UCLI_LOG_INFO, "unknown PHY Link status.\r\n");
			}
			lock_free(ETH_LOCK);
		}
		vTaskDelay(configTICK_RATE_HZ / 5);
	} while (tmp == PHY_LINK_OFF);

	ucli_log(UCLI_LOG_INFO, "waiting for network address..\r\n");
	udp_int_init();

	// wait for IP addres before initialization
	uint32_t taddr = 0;
	do {
		if (lock_take(ETH_LOCK, portMAX_DELAY))
		{
			getSIPR((uint8_t*)&taddr);
			lock_free(ETH_LOCK);
		}
		vTaskDelay(configTICK_RATE_HZ / 5);
	} while (taddr == 0);

	vTaskDelay(500);

	// actual setup
	if (lock_take(ETH_LOCK, portMAX_DELAY))
	{
		prvSetupUDPServer();

		uint16_t imr = IK_SOCK_0 | IK_SOCK_1 | IK_SOCK_2 | IK_SOCK_3;
		if (ctlwizchip(CW_SET_INTRMASK, &imr) == -1) {
			ucli_log(UCLI_LOG_ERROR, "network error, cannot set imr\r\n");
		}

		uint8_t ret = 0;
		for (int i = 0; i < 4; i++)
		{
			uint8_t listen_ret = listen(i);
			ret |= ( listen_ret != 1 ? 1 : 0) << i;
		}
		ucli_log(UCLI_LOG_INFO, "network socket (on port %d) listen %s\r\n", 5000, !ret ? "OK" : "ERROR");

		lock_free(ETH_LOCK);
	}

	for (;;)
	{
		if (xQueueReceive(xTCPServerIRQ, &trg, 100))
		{
			process_server_data();
		}

		//process_server_data();
	}
}
