/*
 * device.c
 *
 *  Created on: Jan 31, 2019
 *      Author: wizath
 */

#include "device.h"
#include "ucli.h"

#include "eeprom.h"
#include "i2c.h"
#include "network.h"
#include "locks.h"

static device_t booster;

void device_read_revision(void)
{
	uint8_t hw_rev = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_2) << 2 | GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_1) << 1 |GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_0);
	booster.hw_rev = hw_rev;
	ucli_log(UCLI_LOG_INFO, "hardware revision %d\r\n", hw_rev);

	// set appropriate resistor values for hardware revision
	if (booster.hw_rev == 1 || booster.hw_rev == 3) {
		booster.p30_current_sense = 0.091f;
	} else {
		booster.p30_current_sense = 0.02f;
	}
}

void device_load_network_conf(void)
{
	if (i2c_device_connected(I2C2, EEPROM_ADDR))
	{
		ucli_log(UCLI_LOG_INFO, "Found mainboard EEPROM, loading values\r\n");

		// if eeprom is empty, it will return 0xFF
		booster.ipsel = eeprom_read_mb(IP_METHOD);
		if (booster.ipsel == 0xFF)
			booster.ipsel = NETINFO_DHCP;

		booster.macsel = eeprom_read_mb(MAC_ADDRESS_SELECT);
		if (booster.macsel == 0xFF)
			booster.macsel = 0;

		for (int i = 0; i < 4; i++) booster.ipaddr[i] = eeprom_read_mb(IP_ADDRESS + i);
		for (int i = 0; i < 4; i++) booster.gwaddr[i] = eeprom_read_mb(IP_ADDRESS_GW + i);
		for (int i = 0; i < 4; i++) booster.netmask[i] = eeprom_read_mb(IP_ADDRESS_NETMASK + i);

		if (booster.macsel == 1) {
			for (int i = 0; i < 6; i++)
				booster.macaddr[i] = eeprom_read_mb(MAC_ADDRESS + i);
		} else {
			for (int i = 0; i < 6; i++) {
				booster.macaddr[i] = STM_GetUniqueID(6 + i);
			}
		}
	} else {
		ucli_log(UCLI_LOG_INFO, "Mainboard EEPROM not found, loading default values\r\n");
		booster.ipsel = NETINFO_DHCP;
		for (int i = 0; i < 6; i++) {
			booster.macaddr[i] = STM_GetUniqueID(6 + i);
		}
	}
}

void device_load_wiznet_conf(wiz_NetInfo * conf)
{
	memcpy(conf->mac, &booster.macaddr, 6);

	if (booster.ipsel == NETINFO_STATIC) {
		conf->dhcp = NETINFO_STATIC;
		memcpy(conf->ip, booster.ipaddr, 4);
		memcpy(conf->gw, booster.gwaddr, 4);
		memcpy(conf->sn, booster.netmask, 4);
	} else {
		conf->dhcp = NETINFO_DHCP;
	}
}

device_t * device_get_config(void)
{
	return &booster;
}
