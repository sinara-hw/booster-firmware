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
	i2c_read_byte_nack(I2C1, &byte);

	i2c_stop(I2C1);

	return byte;
}

void eeprom_write_mb(uint8_t address, uint8_t data)
{
	i2c_start(I2C2, EEPROM_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C2, address & 0xFF);
	i2c_write_byte(I2C2, data);
	i2c_stop(I2C2);
}

uint8_t eeprom_read_mb(uint8_t address)
{
	uint8_t byte;

	i2c_start(I2C2, EEPROM_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C2, address & 0xFF);
	i2c_stop(I2C2);

	i2c_start(I2C2, EEPROM_ADDR, I2C_Direction_Receiver, 1);
	i2c_read_byte_nack(I2C2, &byte);

	i2c_stop(I2C2);

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
	vTaskDelay(50);
	eeprom_write(address + 1, lb);
	vTaskDelay(50);
}

uint32_t eeprom_read32(uint8_t address)
{
	uint8_t bytes[4];
	bytes[0] = eeprom_read(address);
	bytes[1] = eeprom_read(address + 1);
	bytes[2] = eeprom_read(address + 2);
	bytes[3] = eeprom_read(address + 3);

	return ((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}

void eeprom_write32(uint8_t address, uint32_t value)
{
	uint8_t bytes[4];
	bytes[0] = (value >> 24) & 0xFF;
	bytes[1] = (value >> 16) & 0xFF;
	bytes[2] = (value >> 8) & 0xFF;
	bytes[3] = value & 0xFF;

	eeprom_write(address, bytes[0]);
	vTaskDelay(10);
	eeprom_write(address + 1, bytes[1]);
	vTaskDelay(10);
	eeprom_write(address + 2, bytes[2]);
	vTaskDelay(10);
	eeprom_write(address + 3, bytes[3]);
	vTaskDelay(10);
}
