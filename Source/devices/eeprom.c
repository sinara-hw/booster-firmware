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

uint16_t eeprom_read16(uint8_t address)
{
	uint8_t hb = eeprom_read(address);
	uint8_t lb = eeprom_read(address + 1);
	return (hb << 8) | lb;
}

void eeprom_write16(uint8_t address, uint16_t value)
{
	uint8_t lb = value & 0xFF;
	uint8_t hb = (value >> 8) & 0xFF;

	eeprom_write(address, hb);
	vTaskDelay(10);
	eeprom_write(address + 1, lb);
	vTaskDelay(10);
}


