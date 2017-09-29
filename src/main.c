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

#include "scpi/scpi.h"
#include "scpi-def.h"

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

user_data_t user_data = {
    .ipsrc = NULL,
    .ipsrc_port = NULL,
};

size_t SCPI_Write(scpi_t * context, const char * data, size_t len)
{
	if (context->user_context != NULL) {
		user_data_t * u = (user_data_t *) (context->user_context);
		if (u->ipsrc) return sendto(0, (uint8_t *) data, len, u->ipsrc, u->ipsrc_port);
	}
	return 0;
}

scpi_result_t SCPI_Flush(scpi_t * context)
{
    if (context->user_context != NULL) {
//        user_data_t * u = (user_data_t *) (context->user_context);
		/* flush not implemented */
		return SCPI_RES_OK;
    }
    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;
    /* BEEP */
    printf("**ERROR: %ld, \"%s\"\r\n", (int32_t) err, SCPI_ErrorTranslate(err));
    if (err != 0) {
        /* New error */
        /* Beep */
        /* Error LED ON */
    } else {
        /* No more errors in the queue */
        /* Error LED OFF */
    }
    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    char b[16];

    if (SCPI_CTRL_SRQ == ctrl) {
        printf("**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        printf("**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }

    if (context->user_context != NULL) {
        user_data_t * u = (user_data_t *) (context->user_context);
        if (u->ipsrc) {
            snprintf(b, sizeof (b), "SRQ%d\r\n", val);
            return sendto(0, b, strlen(b), u->ipsrc, u->ipsrc_port);
        }
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;
    iprintf("**Reset\r\n");
    return SCPI_RES_OK;
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

    SCPI_Init(&scpi_context,
				scpi_commands,
				&scpi_interface,
				scpi_units_def,
				SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
				scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
				scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

    scpi_context.user_context = &user_data;

	xTaskCreate(prvLEDTask, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(prvDHCPTask, "DHCP", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, &xDHCPTask);
	xTaskCreate(vCommandConsoleTask, "CLI", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	vRegisterCLICommands();

	/* Start the scheduler */
	vTaskStartScheduler();

	return 1;
}
