/*
 * cli.c
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */

#include "cli.h"

#include "network.h"
#include "usb.h"
#include "i2c.h"
#include "channels.h"
#include "locks.h"
#include "eeprom.h"
#include "led_bar.h"
#include "tasks.h"
#include "ads7924.h"
#include "math.h"

extern xQueueHandle _xRxQueue;
extern int __io_putchar(int ch);
static ucli_cmd_t g_cmds[];


static void fh_reboot(void * a_data)
{
	NVIC_SystemReset();
}


static void fh_start(void * a_data)
{
	vTaskResume(task_rf_info);
}


static void fh_stop(void * a_data)
{
	vTaskSuspend(task_rf_info);
}


static void fh_sw_version(void * a_data)
{
	printf("Software information:\r\nBuilt %s %s\r\nFor hardware revision: v%0.2f\r\n", __DATE__, __TIME__, 1.1f);
}


static void fh_bootloader(void * a_data)
{
	usb_enter_bootloader();
}


static void fh_enable(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	int channel_mask = 0;
	ucli_param_get_int(a_ctx, 1, &channel_mask);

	rf_channels_enable((uint8_t) channel_mask);
	printf("[enable] Enabled %d channel mask\r\n", (uint8_t) channel_mask);
}


static void fh_disable(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	int channel_mask = 0;
	ucli_param_get_int(a_ctx, 1, &channel_mask);

	rf_channels_disable((uint8_t) channel_mask);
	printf("[disable] Disabled %d channel mask\r\n", (uint8_t) channel_mask);
}


static void fh_eepromr(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	int channel = 0;
	int address = 0;

	ucli_param_get_int(a_ctx, 1, &channel);
	ucli_param_get_int(a_ctx, 2, &address);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			printf("[eepromr] EEPROM reg[%d] = %d\r\n", (uint8_t) address, eeprom_read((uint8_t) address));
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[eepromr] Wrong channel number\r\n");
	}
}


static void fh_eepromw(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	int channel = 0;
	int address = 0;
	int data = 0;

	ucli_param_get_int(a_ctx, 1, &channel);
	ucli_param_get_int(a_ctx, 2, &address);
	ucli_param_get_int(a_ctx, 3, &data);

	if ((uint8_t) channel < 8)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			i2c_mux_select((uint8_t) channel);
			eeprom_write(address, (uint8_t) data);
			vTaskDelay(10);
			printf("[eepromw] EEPROM reg[%d] = %d\r\n", (uint8_t) address, eeprom_read((uint8_t) address));
			lock_free(I2C_LOCK);
		}
	} else {
		printf("[eepromw] Wrong channel number\r\n");
	}
}


static void fh_taskstats(void * a_data)
{
	char buf[512] = { 0 };
	const char *const pcHeader = "Task\t\tState\tPriority\tStack\t#\r\n**************************************************\r\n";

	strcpy( buf, pcHeader );
	vTaskList( buf + strlen( pcHeader ) );
	puts(buf);
}


static void fh_netconfig(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	uint8_t ipaddr[15] = { 0 };
	uint8_t ipdst[15] = { 0 };
	uint8_t subnet[15] = { 0 };

	if (a_ctx->argc == 1) {
		display_net_conf();
	} else if (a_ctx->argc == 2) {
		if (!strcmp(a_ctx->argv[1], "dhcp")) {
			prvDHCPTaskRestart();
			printf("[ipconfig] dhcp\r\n");
		}
	} else if (a_ctx->argc == 4) {
		uint8_t check = 1;

		if (prvCheckValidIPAddress(a_ctx->argv[1], ipaddr) != 4) check = 0;
		if (prvCheckValidIPAddress(a_ctx->argv[2], ipdst) != 4) check = 0;
		if (prvCheckValidIPAddress(a_ctx->argv[3], subnet) != 4) check = 0;

		if (check)
		{
			set_net_conf(ipaddr, ipdst, subnet);
			printf("[ipconfig] static %s %s %s\r\n", ipaddr, ipdst, subnet);
		} else
			printf("[ipconfig] error while parsing parameters\r\n");
	}
}

static void fh_macconfig(void * a_data)
{
	ucli_ctx_t * a_ctx = (ucli_ctx_t *) a_data;
	uint8_t check = 1;
	uint8_t macdata[6];

	if (!strcmp(a_ctx->argv[1], "default"))
	{
		set_default_mac_address();
		printf("[macconfig] Default MAC address set\r\n");
		return;
	}

	if (prvCheckValidMACAddress(a_ctx->argv[1], macdata) != 6) check = 0;

	if (check) {
		printf("[macconfig] MAC address %s set\r\n", a_ctx->argv[1]);
		set_mac_address(macdata);
	} else
		printf("[macconfig] Wrong MAC address specified\r\n");
}

void vCommandConsoleTask(void *pvParameters)
{
	char ch;

	memset(&ucli_ctx, 0x00, sizeof(ucli_ctx_t));
	ucli_ctx.printfn = (void*) __io_putchar;
	ucli_init(&ucli_ctx, g_cmds);

	for (;;)
	{
		if (xQueueReceive(_xRxQueue, &ch, portMAX_DELAY))
		{
			ucli_process_chr(&ucli_ctx, ch);
		}
	}
}


static ucli_cmd_t g_cmds[] = {
	{ "reboot", fh_reboot, 0x00, "Reboot device\r\n" },
	{ "version", fh_sw_version, 0x00, "Print software version information\r\n" },
	{ "bootloader", fh_bootloader, 0x00, "Enter DFU bootloader\r\n" },
	{ "eepromr", fh_eepromr, 0x02, "Read module EEPROM address\r\n" },
	{ "eepromw", fh_eepromw, 0x03, "Write module EEPROM address\r\n" },
	{ "task-stats", fh_taskstats, 0x00, "Display task info\r\n" },
	{ "start", fh_start, 0x00, "Start the watch thread\r\n" },
	{ "stop", fh_stop, 0x00, "Stop the watch thread\r\n" },
	{ "ifconfig", fh_netconfig, -1, "Change or display network settings\r\n" },
	{ "macconfig", fh_macconfig, 0x01, "Change network MAC address\r\n" },
	{ "enable", fh_enable, 0x01, "Enable selected channel mask\r\n" },
	{ "disable", fh_disable, 0x01, "Disable selected channel mask\r\n" },

    // null
    { 0x00, 0x00, 0x00  }
};
