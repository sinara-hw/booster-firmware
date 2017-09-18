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

// board drivers
#include "ads7924.h"

void gpio_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/* Configure PG6 and PG8 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* Configure PG6 and PG8 in output pushpull mode */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Configure PG6 and PG8 in output pushpull mode */
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

	// Enable channel 7 pwr
//	GPIO_SetBits(GPIOD, GPIO_Pin_7);
}



static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	gpio_init();
	uart_init();
	init_i2c();
}

void prvLEDTask(void *pvParameters)
{
	for (;;)
	{
		GPIO_ToggleBits(BOARD_LED1);
		vTaskDelay(1000);
	}
}

int main(void)
{
	prvSetupHardware();

	i2c_mux_select(0);
	i2c_scan_devices();

//	ads7924_init();

	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

	/* Start the scheduler. */
	vTaskStartScheduler();

	return 1;

	for(;;)
	{
//		uint16_t dt[4] = { 0 };
//		ads7924_get_data(dt);
//		printf("--------\n");
//		printf("ch0 %d\n", dt[0]);
//		printf("ch1 %d\n", dt[1]);
//		printf("ch2 %d\n", dt[2]);
//		printf("ch3 %d\n", dt[3]);
//		for (int i = 0; i < 8400000; i++){};
	}
}
