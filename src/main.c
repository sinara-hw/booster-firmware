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

void gpio_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_8);
	GPIO_SetBits(GPIOC, GPIO_Pin_9);
	GPIO_SetBits(GPIOC, GPIO_Pin_10);

	// ADC reset off
	GPIO_SetBits(GPIOB, GPIO_Pin_9);
}

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	gpio_init();
	uart_init();
	i2c_init();
	spi_init();
}


void prvLEDTask(void *pvParameters)
{
	for (;;)
	{
		GPIO_ToggleBits(BOARD_LED2);
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
	printf("[adc_test] %s | raw: %d | VrefInt %.2f V\n", val == 0 ? "fail" : "success", val, (float) ((val * 2.5) / 4096));

	i2c_mux_select(0);
	i2c_scan_devices();

//	ads7924_init();
	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);

	/* Start the scheduler. */
	vTaskStartScheduler();

	return 1;

	for(;;)
	{
	}
}
