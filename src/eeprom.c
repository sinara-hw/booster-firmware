/*
 * eeprom.c
 *
 *  Created on: Jul 4, 2018
 *      Author: wizath
 */

#include "config.h"
#include "eeprom.h"
#include "i2c.h"

void eeprom_write(uint8_t address, uint8_t data)
{
	i2c_start(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, address);
	i2c_write_byte(I2C1, data);
	i2c_stop(I2C1);
}

uint8_t eeprom_read(uint8_t address)
{
	uint8_t byte;

	i2c_start(I2C1, EEPROM_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, address);
	i2c_stop(I2C1);

	i2c_start(I2C1, EEPROM_ADDR, I2C_Direction_Receiver, 1);
	byte = i2c_read_byte_nack(I2C1);

	i2c_stop(I2C1);

	return byte;
}


