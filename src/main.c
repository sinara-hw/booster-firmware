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
#include "cli.h"
#include "protocol.h"
#include "channels.h"
#include "max6639.h"

// usb
#include "usb.h"

__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END ;
USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,


  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};

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
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	/* PGOOD */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_8);
	GPIO_SetBits(GPIOC, GPIO_Pin_9);
	GPIO_SetBits(GPIOC, GPIO_Pin_10);
}

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	gpio_init();
	uart_init();
	i2c_init();
	spi_init();

//	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
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

//	printf("ret %d\n", spi_receive_byte());

	xEthInterfaceAvailable = xSemaphoreCreateMutex();
	xSemaphoreGive(xEthInterfaceAvailable);

//	rf_channels_init();

	uint16_t val = adc_autotest();
	printf("[test] ADC: %s | raw: %d | VrefInt %.2f V\n", val == 0 ? "ERROR" : "OK", val, (float) ((val * 2.5) / 4096));
	printf("[test] PGOOD: %s\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4) ? "OK" : "ERROR");

//	for (int i = 0; i < 8; i++) {
//		printf("[test] scanning channel %d\n", i);
//		i2c_mux_select(i);
//		i2c_scan_devices();
//	}

//	max6639_init();
//	ads7924_init();
//	scpi_init();
//	adc_init();

	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
//	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
//	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
//	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
