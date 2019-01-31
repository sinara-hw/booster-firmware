/*
 * device.c
 *
 *  Created on: Jan 31, 2019
 *      Author: wizath
 */

#include "device.h"
#include "ucli.h"

typedef struct {
	uint8_t hw_rev;
	double p30_current_sense;
	double sw_ovc_current_value;

} device_t;

static device_t booster;

void device_read_revision(void)
{
	uint8_t hw_rev = GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_2) << 2 | GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_1) << 1 |GPIO_ReadInputDataBit(GPIOF, GPIO_Pin_0);
	booster.hw_rev = hw_rev;
	ucli_log(UCLI_LOG_INFO, "hardware revision %d\r\n", hw_rev);

	// set appropriate resistor values for hardware revision
	if (booster.hw_rev == 2)
		booster.p30_current_sense = 0.091f;
	else
		booster.p30_current_sense = 0.1f;
}

device_t * device_get_config(void)
{
	return &booster;
}
