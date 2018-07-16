/*
 * network.c
 *
 *  Created on: Aug 1, 2017
 *      Author: wizath
 */

#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

#include "config.h"
#include "spi.h"
#include "dhcp.h"
#include "network.h"
#include "server.h"
#include "tasks.h"
#include "locks.h"

wiz_NetInfo gWIZNETINFO_default = { .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
									.ip = {192, 168, 1, 10},
									.sn = {255, 255, 255, 0},
									.gw = {192, 168, 1, 1},
									.dns = {0, 0, 0, 0},
									.dhcp = NETINFO_STATIC
						          };

wiz_NetInfo gWIZNETINFO = { .mac = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
                            .ip = {192, 168, 1, 10},
                            .sn = {255, 255, 255, 0},
                            .gw = {192, 168, 1, 1},
                            .dns = {0, 0, 0, 0},
                            .dhcp = NETINFO_DHCP
						  };

TaskHandle_t xDHCPTask;
TaskHandle_t xUDPServerTask;

uint8_t dhcp_buf[1024] = { 0 };

void wizchip_select(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
}

void wizchip_deselect(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_4);
}

void wizchip_write(uint8_t wb)
{
	spi_send_byte(wb);
}

uint8_t wizchip_read()
{
	return spi_receive_byte();
}

void net_init(void)
{
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};
	uint8_t tmp = 0;

	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

	/* WIZCHIP SOCKET Buffer initialize */
	if(ctlwizchip(CW_INIT_WIZCHIP,(void*)memsize) == -1)
	{
		printf("[dbg] WIZCHIP Init fail.\r\n");
		while(1);
	}

	/* PHY link status check */
	do
	{
		if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
			printf("[dbg] Unknown PHY Link status.\r\n");
		vTaskDelay(configTICK_RATE_HZ / 10);
	} while (tmp == PHY_LINK_OFF);
}

static void dhcp_timer_init(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	/* TIM3 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Enable the TIM3 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	uint16_t PrescalerValue = (uint16_t) ((SystemCoreClock) / 100) - 1;

	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Period = 1000 - 1;
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_PrescalerConfig(TIM3, PrescalerValue, TIM_PSCReloadMode_Immediate);

	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
}

void TIM3_IRQHandler()
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
    {
        DHCP_time_handler();
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
    }
}

void set_net_conf(uint8_t * ipsrc, uint8_t * ipdst, uint8_t * subnet)
{
	gWIZNETINFO.ip[0] = ipsrc[0];
	gWIZNETINFO.ip[1] = ipsrc[1];
	gWIZNETINFO.ip[2] = ipsrc[2];
	gWIZNETINFO.ip[3] = ipsrc[3];

	gWIZNETINFO.gw[0] = ipdst[0];
	gWIZNETINFO.gw[1] = ipdst[1];
	gWIZNETINFO.gw[2] = ipdst[2];
	gWIZNETINFO.gw[3] = ipdst[3];

	gWIZNETINFO.sn[0] = subnet[0];
	gWIZNETINFO.sn[1] = subnet[1];
	gWIZNETINFO.sn[2] = subnet[2];
	gWIZNETINFO.sn[3] = subnet[3];

	gWIZNETINFO.dhcp = NETINFO_STATIC;
	prvDHCPTaskStop();

	net_conf(&gWIZNETINFO);
	display_net_conf();
	prvRestartServerTask();
}

void net_conf(wiz_NetInfo * netinfo)
{
	/* wizchip netconf */
	ctlnetwork(CN_SET_NETINFO, (void*) netinfo);
}

void display_net_conf(void)
{
	uint8_t tmpstr[6] = {0,};
	wiz_NetInfo glWIZNETINFO;

	ctlnetwork(CN_GET_NETINFO, (void*) &glWIZNETINFO);
	// Display Network Information
	ctlwizchip(CW_GET_ID,(void*)tmpstr);

	if(glWIZNETINFO.dhcp == NETINFO_DHCP)
		printf("[log] ===== %s NET CONF : DHCP =====\r\n",(char*)tmpstr);
	else
		printf("[log] ===== %s NET CONF : Static =====\r\n",(char*)tmpstr);

	printf("[log]\tMAC:\t%02X:%02X:%02X:%02X:%02X:%02X\r\n", glWIZNETINFO.mac[0], glWIZNETINFO.mac[1], glWIZNETINFO.mac[2], glWIZNETINFO.mac[3], glWIZNETINFO.mac[4], glWIZNETINFO.mac[5]);
	printf("[log]\tIP:\t%d.%d.%d.%d\r\n", glWIZNETINFO.ip[0], glWIZNETINFO.ip[1], glWIZNETINFO.ip[2], glWIZNETINFO.ip[3]);
	printf("[log]\tGW:\t%d.%d.%d.%d\r\n", glWIZNETINFO.gw[0], glWIZNETINFO.gw[1], glWIZNETINFO.gw[2], glWIZNETINFO.gw[3]);
	printf("[log]\tSN:\t%d.%d.%d.%d\r\n", glWIZNETINFO.sn[0], glWIZNETINFO.sn[1], glWIZNETINFO.sn[2], glWIZNETINFO.sn[3]);
	printf("[log] =======================================\r\n");
}

void prvRestartServerTask(void)
{
	if (xUDPServerTask == NULL) {
		// create server task
		printf("[log] created udp server task\n");
		xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask);
	} else {
		// restart server task
		printf("[log] restarted udp server task\n");
		vTaskDelete(xUDPServerTask);
		xUDPServerTask = NULL;
		xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask);
	}
}

void ldhcp_ip_assign(void)
{
   getIPfromDHCP(gWIZNETINFO.ip);
   getGWfromDHCP(gWIZNETINFO.gw);
   getSNfromDHCP(gWIZNETINFO.sn);
   getDNSfromDHCP(gWIZNETINFO.dns);
   gWIZNETINFO.dhcp = NETINFO_DHCP;

   /* Network initialization */
   net_conf(&gWIZNETINFO);
   display_net_conf();
   printf("[log] DHCP lease time: %ld s\n", getDHCPLeasetime());

   prvRestartServerTask();
}

void ldhcp_ip_conflict(void)
{
	printf("[log] DHCP IP Conflict\n");

	// restart DHCP
	net_init();
	DHCP_init(DHCP_SOCKET, dhcp_buf);
	reg_dhcp_cbfunc(ldhcp_ip_assign, ldhcp_ip_assign, ldhcp_ip_conflict);
}

void prvDHCPTaskStart(void)
{
	printf("Re/Starting DHCP client");
	if (xDHCPTask != NULL) vTaskDelete(xDHCPTask);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
}

void prvDHCPTaskStop(void)
{
	DHCP_stop();
	if (xDHCPTask != NULL) vTaskDelete(xDHCPTask);
	xDHCPTask = NULL;

	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
}

void prvDHCPTaskRestart(void)
{
	prvDHCPTaskStop();
	prvDHCPTaskStart();
}

void prvDHCPTask(void *pvParameters)
{
	uint8_t ldhcp_retry_count = 0;

	net_init();
	DHCP_init(DHCP_SOCKET, dhcp_buf);
	reg_dhcp_cbfunc(ldhcp_ip_assign, ldhcp_ip_assign, ldhcp_ip_conflict);
	dhcp_timer_init();

	for (;;)
	{
		if (lock_take(ETH_LOCK, portMAX_DELAY)) {
			switch(DHCP_run())
			{
				case DHCP_FAILED:
					if (++ldhcp_retry_count > DHCP_MAX_RETRIES)
					{
						printf("[log] DHCP failed after %d retries\n", ldhcp_retry_count);
						DHCP_stop();
						net_conf(&gWIZNETINFO_default);
						ldhcp_retry_count = 0;
						printf("[log] Loaded static IP settings\n");
						display_net_conf();
						vTaskDelete(NULL);
					}
					break;
				default:
					break;
			}

			vTaskDelay(configTICK_RATE_HZ / 5);
			lock_free(ETH_LOCK);
		}
	}
}
