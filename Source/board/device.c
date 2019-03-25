/*
 * device.c
 *
 *  Created on: Jan 31, 2019
 *      Author: wizath
 */

#include "device.h"
#include "ucli.h"

static device_t booster;

void device_read_revision(void)
{
	uint8_t hw_rev = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_2) << 2 | GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_1) << 1 |GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_0);
	booster.hw_rev = hw_rev;
	ucli_log(UCLI_LOG_INFO, "hardware revision %d\r\n", hw_rev);

	// set appropriate resistor values for hardware revision
	if (booster.hw_rev == 1 || booster.hw_rev == 3) {
		booster.p30_current_sense = 0.091f;
		// since 2.275V on ADC == 0.5 A current on P30V
	} else {
		booster.p30_current_sense = 0.02f;
		// since 0.5V on ADC == 0.5 A current on P30V
	}
}

void device_read_mac_eeprom(void)
{

}

device_t * device_get_config(void)
{
	return &booster;
}
