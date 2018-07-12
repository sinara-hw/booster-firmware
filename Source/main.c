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

// usb
#include "usb.h"

void gpio_init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
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

	/* I2C switch reset */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_14);

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
	lock_init();

	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);
	printf("[log] device boot\n");
	printf("[log] SYSCLK frequency: %lu\n", RCC_ClockFreq.SYSCLK_Frequency);
	printf("[log] PCLK1 frequency: %lu\n", RCC_ClockFreq.PCLK1_Frequency);
	printf("[log] PCLK2 frequency: %lu\n", RCC_ClockFreq.PCLK2_Frequency);
}

extern channel_t channels[8];
extern volatile uint8_t channel_mask;
extern volatile uint8_t f_speed;
volatile float fTemp, fAvg, chTemp;

void prvLEDTask(void *pvParameters)
{
	uint8_t address_list[] = {0x2C, 0x2E, 0x2F};

    for (;;)
    {
//    	puts("\033[2J");
    	printf("PGOOD: %d\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4));
    	printf("TEMPERATURES\n");
    	for (int i = 0; i < 3; i ++)
		{
    		if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				float temp1 = max6639_get_temperature(address_list[i], 0);
				float temp2 = max6639_get_temperature(address_list[i], 1);

				lock_free(I2C_LOCK);

				printf("[%d] ch1 (ext): %.3f ch2 (int): %.3f\n", i, temp1, temp2);
			}
		}

    	printf("FAN SPEED: %d %%\n", f_speed);
    	printf("AVG TEMP: %0.2f MAX:%0.2f\n", fAvg, chTemp);
    	printf("CHANNELS INFO\n");
        printf("==============================================================================\n");
        printf("\t\t#0\t#1\t#2\t#3\t#4\t#5\t#6\t#7\n");

        printf("DETECTED\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].detected,
               channels[1].detected,
               channels[2].detected,
               channels[3].detected,
               channels[4].detected,
               channels[5].detected,
               channels[6].detected,
               channels[7].detected);

        printf("TXPWR [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].adc_ch1,
               channels[1].adc_ch1,
               channels[2].adc_ch1,
               channels[3].adc_ch1,
               channels[4].adc_ch1,
               channels[5].adc_ch1,
               channels[6].adc_ch1,
               channels[7].adc_ch1);

        printf("RFLPWR [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].adc_ch2,
               channels[1].adc_ch2,
               channels[2].adc_ch2,
               channels[3].adc_ch2,
               channels[4].adc_ch2,
               channels[5].adc_ch2,
               channels[6].adc_ch2,
               channels[7].adc_ch2);

//        printf("I30V [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].pwr_ch1,
//               channels[1].pwr_ch1,
//               channels[2].pwr_ch1,
//               channels[3].pwr_ch1,
//               channels[4].pwr_ch1,
//               channels[5].pwr_ch1,
//               channels[6].pwr_ch1,
//               channels[7].pwr_ch1);

        printf("P30V [A]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].i30,
			   channels[1].i30,
			   channels[2].i30,
			   channels[3].i30,
			   channels[4].i30,
			   channels[5].i30,
			   channels[6].i30,
			   channels[7].i30);

//        printf("I6V0 [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].pwr_ch2,
//               channels[1].pwr_ch2,
//               channels[2].pwr_ch2,
//               channels[3].pwr_ch2,
//               channels[4].pwr_ch2,
//               channels[5].pwr_ch2,
//               channels[6].pwr_ch2,
//               channels[7].pwr_ch2);

        printf("P6V0 [A]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].i60,
			   channels[1].i60,
			   channels[2].i60,
			   channels[3].i60,
			   channels[4].i60,
			   channels[5].i60,
			   channels[6].i60,
			   channels[7].i60);

//        printf("IN8V0 [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].pwr_ch3,
//               channels[1].pwr_ch3,
//               channels[2].pwr_ch3,
//               channels[3].pwr_ch3,
//               channels[4].pwr_ch3,
//               channels[5].pwr_ch3,
//               channels[6].pwr_ch3,
//               channels[7].pwr_ch3);

        printf("PN8V0 [mA]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].in80 * 1000,
			   channels[1].in80 * 1000,
			   channels[2].in80 * 1000,
			   channels[3].in80 * 1000,
			   channels[4].in80 * 1000,
			   channels[5].in80 * 1000,
			   channels[6].in80 * 1000,
			   channels[7].in80 * 1000);

        printf("P3V3MP [V]\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t%0.2f\t\n", channels[0].pwr_ch4,
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

        printf("DAC1\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].dac1_value,
                       channels[1].dac1_value,
                       channels[2].dac1_value,
                       channels[3].dac1_value,
                       channels[4].dac1_value,
                       channels[5].dac1_value,
                       channels[6].dac1_value,
                       channels[7].dac1_value);

        printf("DAC2\t\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n", channels[0].dac2_value,
                       channels[1].dac2_value,
                       channels[2].dac2_value,
                       channels[3].dac2_value,
                       channels[4].dac2_value,
                       channels[5].dac2_value,
                       channels[6].dac2_value,
                       channels[7].dac2_value);

        printf("LTEMP\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].local_temp,
                       channels[1].local_temp,
                       channels[2].local_temp,
                       channels[3].local_temp,
                       channels[4].local_temp,
                       channels[5].local_temp,
                       channels[6].local_temp,
                       channels[7].local_temp);

        printf("RTEMP\t\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t\n", channels[0].remote_temp,
					   channels[1].remote_temp,
					   channels[2].remote_temp,
					   channels[3].remote_temp,
					   channels[4].remote_temp,
					   channels[5].remote_temp,
					   channels[6].remote_temp,
					   channels[7].remote_temp);

        printf("==============================================================================\n");

    	GPIO_ToggleBits(BOARD_LED3);
        vTaskDelay(1000);
    }
}

void prvLEDTask2(void *pvParameters)
{
	for (;;)
	{
		vTaskDelay(500);
	}
}

extern SemaphoreHandle_t xEthInterfaceAvailable;
extern TaskHandle_t	xDHCPTask;

TaskHandle_t xChannelTask = NULL;
TaskHandle_t xStatusTask = NULL;

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

	xTaskCreate(prvTempMgtTask, "FAN", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY, &xStatusTask);
	xTaskCreate(prvADCTask, "ADC", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 3, NULL);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate(prcRFChannelsTask, "RFCH", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, &xChannelTask );
	xTaskCreate(prvExtTask, "Ext", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
