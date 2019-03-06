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
#include "stm32f4xx_iwdg.h"

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
#include "ucli.h"
#include "device.h"

// usb
#include "usb.h"

static void watchdog_init(void)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(IWDG_Prescaler_32); // 1024 Hz clock
	IWDG_SetReload(0x1400); // 2 second reset timeout
	IWDG_ReloadCounter();
	IWDG_Enable();
}

static void prvSetupHardware(void)
{
	SystemCoreClockUpdate();

	usb_init();
	cli_init();
	gpio_init();
	i2c_init();
	spi_init();
	ext_init();
	lock_init();
	watchdog_init();

	max6639_init();
	led_bar_init();
	scpi_init();

	device_read_revision();

	RCC_ClocksTypeDef RCC_ClockFreq;
	RCC_GetClocksFreq(&RCC_ClockFreq);

	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
		// watchdog reset occurred
		ucli_log(UCLI_LOG_WARN, "watchdog reset occurred!\r\n");
		RCC_ClearFlag();
	}

	ucli_log(UCLI_LOG_INFO, "device boot\r\n");
	ucli_log(UCLI_LOG_INFO, "SYSCLK frequency: %lu\r\n", RCC_ClockFreq.SYSCLK_Frequency);
	ucli_log(UCLI_LOG_INFO, "PCLK1 frequency: %lu\r\n", RCC_ClockFreq.PCLK1_Frequency);
	ucli_log(UCLI_LOG_INFO, "PCLK2 frequency: %lu\r\n", RCC_ClockFreq.PCLK2_Frequency);
}

typedef struct {
	double input;
	double output;
	double setpoint;

	double kp;
	double ki;
	double kd;
	int pon;

	double output_sum;
	double last_input;
	double out_min;
	double out_max;

	int dir;
} temp_pid_t;

void prvLEDTask(void * pvParameters)
{

}

int main(void)
{
	prvSetupHardware();

	uint16_t val = adc_autotest();
	ucli_log(UCLI_LOG_INFO, "ADC: %s | raw: %d | VrefInt %.2f V\r\n", val == 0 ? "ERROR" : "OK", val, (float) ((val * 2.5) / 4096));
	ucli_log(UCLI_LOG_INFO, "PGOOD: %s\r\n", GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_4) ? "OK" : "ERROR");

	xTaskCreate(prvTempMgtTask, "FAN", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(prvADCTask, "ADC", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 3, NULL);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(prvUDPServerTask, "UDPServer", configMINIMAL_STACK_SIZE + 2048UL, NULL, tskIDLE_PRIORITY + 2, &xUDPServerTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE + 256UL, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate(prvExtTask, "Ext", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	xTaskCreate(rf_channels_interlock_task, "RF Interlock", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 4, &task_rf_interlock );

	/* Start the scheduler */
 	vTaskStartScheduler();

	return 1;
}
