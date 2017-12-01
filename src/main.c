/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/

#include "config.h"
#include "stm32f4xx_rcc.h"

// platform drivers
#include "uart.h"
#include "i2c.h"
#include "spi.h"
#include "adc.h"

// board drivers
#include "ads7924.h"
#include "wizchip_conf.h"
#include "network.h"
#include "dhcp.h"
#include "server.h"
#include "cli.h"
#include "protocol.h"
#include "channels.h"
#include "max6639.h"
#include "usb.h"

// usb
#include "usb.h"

void gpio_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* PGOOD */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOC, GPIO_Pin_8);
	GPIO_ResetBits(GPIOC, GPIO_Pin_9);
	GPIO_ResetBits(GPIOC, GPIO_Pin_10);
}

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	usb_init();
	gpio_init();
//	uart_init();
	i2c_init();
	spi_init();

	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);
	printf("[log] device boot\n");
	printf("[log] SYSCLK frequency: %lu\n", RCC_ClockFreq.SYSCLK_Frequency);
	printf("[log] PCLK1 frequency: %lu\n", RCC_ClockFreq.PCLK1_Frequency);
	printf("[log] PCLK2 frequency: %lu\n", RCC_ClockFreq.PCLK2_Frequency);
}


void prvLEDTask(void *pvParameters)
{
	for (;;)
	{
		GPIO_ToggleBits(BOARD_LED3);
		vTaskDelay(500);
	}
}

extern SemaphoreHandle_t xEthInterfaceAvailable;
extern TaskHandle_t	xDHCPTask;

int main(void)
{
	prvSetupHardware();

	xEthInterfaceAvailable = xSemaphoreCreateMutex();
	xSemaphoreGive(xEthInterfaceAvailable);

	uint16_t val = adc_autotest();
	printf("[test] ADC: %s | raw: %d | VrefInt %.2f V\n", val == 0 ? "ERROR" : "OK", val, (float) ((val * 2.5) / 4096));
	printf("[test] PGOOD: %s\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4) ? "OK" : "ERROR");
	if (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4)) GPIO_SetBits(BOARD_LED1);

//	for (int i = 0; i < 8; i++) {
//		printf("[test] scanning channel %d\n", i);
//		i2c_mux_select(i);
//		i2c_scan_devices();
//	}

//	max6639_init();
//	ads7924_init();
//	scpi_init();
	adc_init();

	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
//	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate(prcRFChannelsTask, "RFCH", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
