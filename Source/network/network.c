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
#include "eeprom.h"
#include "ucli.h"

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

void load_network_values(wiz_NetInfo *glWIZNETINFO)
{
	uint8_t ipaddr[4] = { 0 };
	uint8_t ipaddrgw[4] = { 0 };
	uint8_t ipaddrsn[4] = { 0 };
	uint8_t macaddr[6] = { 0 };
	uint8_t ipsel = 0;
	uint8_t macsel = 0;

	if (i2c_device_connected(I2C2, EEPROM_ADDR))
	{
		ucli_log(UCLI_LOG_INFO, "Found mainboard EEPROM, loading values\r\n");

		ipsel = eeprom_read_mb(IP_METHOD);
		macsel = eeprom_read_mb(MAC_ADDRESS_SELECT);
		for (int i = 0; i < 4; i++) ipaddr[i] = eeprom_read_mb(IP_ADDRESS + i);
		for (int i = 0; i < 4; i++) ipaddrgw[i] = eeprom_read_mb(IP_ADDRESS_GW + i);
		for (int i = 0; i < 4; i++) ipaddrsn[i] = eeprom_read_mb(IP_ADDRESS_NETMASK + i);
		for (int i = 0; i < 6; i++) macaddr[i] = eeprom_read_mb(MAC_ADDRESS + i);

		if (ipsel == NETINFO_STATIC) {
			memcpy(glWIZNETINFO->ip, ipaddr, 4);
			memcpy(glWIZNETINFO->gw, ipaddrgw, 4);
			memcpy(glWIZNETINFO->sn, ipaddrsn, 4);
			glWIZNETINFO->dhcp = NETINFO_STATIC;
		} else {
			glWIZNETINFO->dhcp = NETINFO_DHCP;
		}

		if (macsel == 1) {
			memcpy(glWIZNETINFO->mac, macaddr, 6);
		} else {
			glWIZNETINFO->mac[0] = STM_GetUniqueID(6);
			glWIZNETINFO->mac[1] = STM_GetUniqueID(7);
			glWIZNETINFO->mac[2] = STM_GetUniqueID(8);
			glWIZNETINFO->mac[3] = STM_GetUniqueID(9);
			glWIZNETINFO->mac[4] = STM_GetUniqueID(10);
			glWIZNETINFO->mac[5] = STM_GetUniqueID(11);
		}
	} else {
		glWIZNETINFO->dhcp = NETINFO_DHCP;
		glWIZNETINFO->mac[0] = STM_GetUniqueID(6);
		glWIZNETINFO->mac[1] = STM_GetUniqueID(7);
		glWIZNETINFO->mac[2] = STM_GetUniqueID(8);
		glWIZNETINFO->mac[3] = STM_GetUniqueID(9);
		glWIZNETINFO->mac[4] = STM_GetUniqueID(10);
		glWIZNETINFO->mac[5] = STM_GetUniqueID(11);
	}
}

void net_init(void)
{
	uint8_t memsize[2][8] = {{2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2}};
	uint8_t tmp = 0;

	reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);

	/* WIZCHIP SOCKET Buffer initialize */
	if(ctlwizchip(CW_INIT_WIZCHIP,(void*) memsize) == -1)
	{
		ucli_log(UCLI_LOG_ERROR, "WIZCHIP Init fail.\r\n");
		while(1);
	}

	// set default network settings
	wiz_NetInfo glWIZNETINFO;
	load_network_values(&glWIZNETINFO);
	ctlnetwork(CN_SET_NETINFO, &glWIZNETINFO);

	if (glWIZNETINFO.dhcp == NETINFO_STATIC) {
		vTaskSuspend(xDHCPTask);
	}

	/* PHY link status check */
	do
	{
		if(ctlwizchip(CW_GET_PHYLINK, (void*)&tmp) == -1)
			ucli_log(UCLI_LOG_ERROR, "Unknown PHY Link status.\r\n");
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
	wiz_NetInfo glWIZNETINFO;
	ctlnetwork(CN_GET_NETINFO, (void*) &glWIZNETINFO);

	glWIZNETINFO.ip[0] = ipsrc[0];
	glWIZNETINFO.ip[1] = ipsrc[1];
	glWIZNETINFO.ip[2] = ipsrc[2];
	glWIZNETINFO.ip[3] = ipsrc[3];

	glWIZNETINFO.gw[0] = ipdst[0];
	glWIZNETINFO.gw[1] = ipdst[1];
	glWIZNETINFO.gw[2] = ipdst[2];
	glWIZNETINFO.gw[3] = ipdst[3];

	glWIZNETINFO.sn[0] = subnet[0];
	glWIZNETINFO.sn[1] = subnet[1];
	glWIZNETINFO.sn[2] = subnet[2];
	glWIZNETINFO.sn[3] = subnet[3];

	glWIZNETINFO.dhcp = NETINFO_STATIC;
	prvDHCPTaskStop();

	for (int i = 0; i < 4; i++) {
		eeprom_write_mb(IP_ADDRESS + i, ipsrc[i]);
		vTaskDelay(10);
	}
	for (int i = 0; i < 4; i++) {
		eeprom_write_mb(IP_ADDRESS_GW + i, ipdst[i]);
		vTaskDelay(10);
	}

	for (int i = 0; i < 4; i++) {
		eeprom_write_mb(IP_ADDRESS_NETMASK + i, subnet[i]);
		vTaskDelay(10);
	}

	eeprom_write_mb(IP_METHOD, NETINFO_STATIC);
	vTaskDelay(10);

	net_conf(&glWIZNETINFO);
	display_net_conf();
}

void set_default_mac_address(void)
{
	wiz_NetInfo glWIZNETINFO;
	ctlnetwork(CN_GET_NETINFO, (void*) &glWIZNETINFO);

	glWIZNETINFO.mac[0] = STM_GetUniqueID(6);
	glWIZNETINFO.mac[1] = STM_GetUniqueID(7);
	glWIZNETINFO.mac[2] = STM_GetUniqueID(8);
	glWIZNETINFO.mac[3] = STM_GetUniqueID(9);
	glWIZNETINFO.mac[4] = STM_GetUniqueID(10);
	glWIZNETINFO.mac[5] = STM_GetUniqueID(11);

	net_conf(&glWIZNETINFO);

	// set to 1 to enable custom mac addr
	eeprom_write_mb(MAC_ADDRESS_SELECT, 0);
	vTaskDelay(10);

	if (glWIZNETINFO.dhcp == NETINFO_DHCP) {
		prvDHCPTaskRestart();
	}
}

void set_mac_address(uint8_t * macaddress)
{
	wiz_NetInfo glWIZNETINFO;
	ctlnetwork(CN_GET_NETINFO, (void*) &glWIZNETINFO);
	glWIZNETINFO.mac[0] = macaddress[0];
	glWIZNETINFO.mac[1] = macaddress[1];
	glWIZNETINFO.mac[2] = macaddress[2];
	glWIZNETINFO.mac[3] = macaddress[3];
	glWIZNETINFO.mac[4] = macaddress[4];
	glWIZNETINFO.mac[5] = macaddress[5];

	for (int i = 0; i < 6; i++) {
		eeprom_write_mb(MAC_ADDRESS + i, macaddress[i]);
		vTaskDelay(10);
	}

	// set to 1 to enable custom mac addr
	eeprom_write_mb(MAC_ADDRESS_SELECT, 1);
	vTaskDelay(10);

	net_conf(&glWIZNETINFO);

	if (glWIZNETINFO.dhcp == NETINFO_DHCP) {
		prvDHCPTaskRestart();
	}
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
//	if (xUDPServerTask == NULL) {
//		// create server task
//		printf("[log] created udp server task\n");
//		printf("task stat %d\n", xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask));
//	} else {
//		// restart server task
//		printf("[log] restarted udp server task\n");
//		vTaskDelete(xUDPServerTask);
//		xUDPServerTask = NULL;
//		xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask);
//	}
}

void ldhcp_ip_assign(void)
{
	wiz_NetInfo glWIZNETINFO;
	ctlnetwork(CN_GET_NETINFO, (void*) &glWIZNETINFO);

	getIPfromDHCP(glWIZNETINFO.ip);
	getGWfromDHCP(glWIZNETINFO.gw);
	getSNfromDHCP(glWIZNETINFO.sn);
	getDNSfromDHCP(glWIZNETINFO.dns);
	glWIZNETINFO.dhcp = NETINFO_DHCP;

	ucli_log(UCLI_LOG_INFO, "DHCP IP acquired: %d.%d.%d.%d\r\n", glWIZNETINFO.ip[0], glWIZNETINFO.ip[1], glWIZNETINFO.ip[2], glWIZNETINFO.ip[3]);

	/* Network initialization */
	net_conf(&glWIZNETINFO);
//	display_net_conf();
	ucli_log(UCLI_LOG_INFO, "DHCP lease time: %ld s\n", getDHCPLeasetime());

	prvRestartServerTask();
}

void ldhcp_ip_conflict(void)
{
	ucli_log(UCLI_LOG_ERROR, "DHCP IP Conflict\r\n");

	// restart DHCP
	net_init();
	DHCP_init(DHCP_SOCKET, dhcp_buf);
	reg_dhcp_cbfunc(ldhcp_ip_assign, ldhcp_ip_assign, ldhcp_ip_conflict);
}

void prvDHCPTaskStop(void)
{
	DHCP_stop();
	vTaskSuspend(xDHCPTask);

	TIM_ITConfig(TIM3, TIM_IT_Update, DISABLE);
	TIM_Cmd(TIM3, DISABLE);
}

void prvDHCPTaskRestart(void)
{
	DHCP_init(DHCP_SOCKET, dhcp_buf);
	reg_dhcp_cbfunc(ldhcp_ip_assign, ldhcp_ip_assign, ldhcp_ip_conflict);
	dhcp_timer_init();

	vTaskResume(xDHCPTask);
	eeprom_write_mb(IP_METHOD, NETINFO_DHCP);
	vTaskDelay(10);
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
						reset_DHCP_timeout();
						ldhcp_retry_count = 0;
//						ucli_log(UCLI_LOG_ERROR, "DHCP failed after %d retries\r\n", ldhcp_retry_count);
//						net_conf(&gWIZNETINFO_default);
//						ldhcp_retry_count = 0;
//						ucli_log(UCLI_LOG_INFO, "Loaded static IP settings\r\n");
//						display_net_conf();
//						prvDHCPTaskStop();
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

uint8_t prvCheckValidIPAddress(char * ipstr, uint8_t * result)
{
	uint8_t lParameterCount = 0;
	char * ch;
	uint8_t num;

	ch = strtok((char*)ipstr, ".");
	while (ch != NULL) {
		num = atoi(ch);
		if (num >= 0 && num <= 255 && lParameterCount < 4) {
			result[lParameterCount++] = num;
//			printf("IP [%d] = %d\n", lParameterCount, num);
		}
		ch = strtok(NULL, ".");
	}

	return lParameterCount;
}


uint8_t prvCheckValidMACAddress(char * ipstr, uint8_t * result)
{
	uint8_t lParameterCount = 0;
	char * ch;
	uint8_t num;

	ch = strtok((char*)ipstr, ":");
	while (ch != NULL) {
		num = strtol(ch, NULL, 16);
		if (num >= 0 && num <= 255 && lParameterCount < 6) {
			result[lParameterCount++] = num;
//			printf("MAC [%d] = %d\n", lParameterCount, num);
		}
		ch = strtok(NULL, ":");
	}

	return lParameterCount;
}
