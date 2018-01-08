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
#include "externals.h"

// usb
#include "usb.h"

void gpio_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	/* LEDS */
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
	ext_init();

	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);
	printf("[log] device boot\n");
	printf("[log] SYSCLK frequency: %lu\n", RCC_ClockFreq.SYSCLK_Frequency);
	printf("[log] PCLK1 frequency: %lu\n", RCC_ClockFreq.PCLK1_Frequency);
	printf("[log] PCLK2 frequency: %lu\n", RCC_ClockFreq.PCLK2_Frequency);
}

extern channel_t channels[8];

void prvLEDTask(void *pvParameters)
{
    for (;;)
    {
    	printf("PGOOD: %d\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4));
    	printf("CHANNELS INFO\n");
        printf("==========================================================================\n");

        printf("\t\t#1\t#2\t#3\t#4\t#5\t#6\t#7\t#8\n");

        printf("ADC1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].adc_ch1,
               channels[1].adc_ch1,
               channels[2].adc_ch1,
               channels[3].adc_ch1,
               channels[4].adc_ch1,
               channels[5].adc_ch1,
               channels[6].adc_ch1,
               channels[7].adc_ch1);

        printf("ADC2\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].adc_ch2,
               channels[1].adc_ch2,
               channels[2].adc_ch2,
               channels[3].adc_ch2,
               channels[4].adc_ch2,
               channels[5].adc_ch2,
               channels[6].adc_ch2,
               channels[7].adc_ch2);

        printf("PWR1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].pwr_ch1,
               channels[1].pwr_ch1,
               channels[2].pwr_ch1,
               channels[3].pwr_ch1,
               channels[4].pwr_ch1,
               channels[5].pwr_ch1,
               channels[6].pwr_ch1,
               channels[7].pwr_ch1);

        printf("PWR2\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].pwr_ch2,
               channels[1].pwr_ch2,
               channels[2].pwr_ch2,
               channels[3].pwr_ch2,
               channels[4].pwr_ch2,
               channels[5].pwr_ch2,
               channels[6].pwr_ch2,
               channels[7].pwr_ch2);

        printf("PWR3\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].pwr_ch3,
               channels[1].pwr_ch3,
               channels[2].pwr_ch3,
               channels[3].pwr_ch3,
               channels[4].pwr_ch3,
               channels[5].pwr_ch3,
               channels[6].pwr_ch3,
               channels[7].pwr_ch3);

        printf("PWR4\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].pwr_ch4,
               channels[1].pwr_ch4,
               channels[2].pwr_ch4,
               channels[3].pwr_ch4,
               channels[4].pwr_ch4,
               channels[5].pwr_ch4,
               channels[6].pwr_ch4,
               channels[7].pwr_ch4);

        printf("ON\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].enabled,
               channels[1].enabled,
               channels[2].enabled,
               channels[3].enabled,
               channels[4].enabled,
               channels[5].enabled,
               channels[6].enabled,
               channels[7].enabled);

        printf("SON\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].sigon,
               channels[1].sigon,
               channels[2].sigon,
               channels[3].sigon,
               channels[4].sigon,
               channels[5].sigon,
               channels[6].sigon,
               channels[7].sigon);

        printf("OVL\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].overvoltage,
               channels[1].overvoltage,
               channels[2].overvoltage,
               channels[3].overvoltage,
               channels[4].overvoltage,
               channels[5].overvoltage,
               channels[6].overvoltage,
               channels[7].overvoltage);

        printf("ALERT\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].alert,
               channels[1].alert,
               channels[2].alert,
               channels[3].alert,
               channels[4].alert,
               channels[5].alert,
               channels[6].alert,
               channels[7].alert);

        printf("USERIO\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].userio,
               channels[1].userio,
               channels[2].userio,
               channels[3].userio,
               channels[4].userio,
               channels[5].userio,
               channels[6].userio,
               channels[7].userio);

        printf("==========================================================================\n");


        GPIO_ToggleBits(BOARD_LED3);
        vTaskDelay(2000);
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

	max6639_init();
	scpi_init();
	adc_init();

	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
//	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate(prcRFChannelsTask, "RFCH", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL );
	xTaskCreate(prvExtTask, "Ext", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
