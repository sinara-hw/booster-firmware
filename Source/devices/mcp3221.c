/*
 * mpc3221.c
 *
 *  Created on: Sep 4, 2019
 *      Author: wizath
 */

#include "mcp3221.h"
#include "i2c.h"

uint16_t mcp3221_get_data(void)
{
	uint8_t msb, lsb = 0;

	i2c_start(I2C1, MCP3221_ADDR, I2C_Direction_Receiver, 1);
	vTaskDelay(1);
	i2c_read_byte_ack(I2C1, &msb);
	i2c_read_byte_nack(I2C1, &lsb);
	i2c_stop(I2C1);

	return (msb << 8) | lsb;
}


double mcp3221_get_voltage(void)
{
	uint16_t value = mcp3221_get_data();
	return (value * 3.3f) / 4095;
}
