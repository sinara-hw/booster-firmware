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
#include "i2c.h"
#include "spi.h"
#include "adc.h"

#include "tasks.h"

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
#include "externals.h"
#include "locks.h"
#include "led_bar.h"
#include "temp_mgt.h"
#include "gpio.h"

// usb
#include "usb.h"

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	usb_init();
	gpio_init();
	i2c_init();
	spi_init();
	ext_init();
	lock_init();

	max6639_init();
	led_bar_init();
	scpi_init();

	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);
	printf("[log] device boot\n");
	printf("[log] SYSCLK frequency: %lu\n", RCC_ClockFreq.SYSCLK_Frequency);
	printf("[log] PCLK1 frequency: %lu\n", RCC_ClockFreq.PCLK1_Frequency);
	printf("[log] PCLK2 frequency: %lu\n", RCC_ClockFreq.PCLK2_Frequency);
}

void prvLEDTask2(void *pvParameters)
{
	for (;;)
	{
		vTaskDelay(500);
	}
}

int main(void)
{
	prvSetupHardware();

	uint16_t val = adc_autotest();
	printf("[test] ADC: %s | raw: %d | VrefInt %.2f V\n", val == 0 ? "ERROR" : "OK", val, (float) ((val * 2.5) / 4096));
	printf("[test] PGOOD: %s\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4) ? "OK" : "ERROR");
	if (GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4)) GPIO_SetBits(BOARD_LED1);

	xTaskCreate(prvTempMgtTask, "FAN", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(prvADCTask, "ADC", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 3, NULL);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate(prvExtTask, "Ext", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	xTaskCreate(rf_channels_interlock_task, "RF Prot", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, &task_rf_interlock );
	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
