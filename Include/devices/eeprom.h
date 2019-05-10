/*
 * eeprom.h
 *
 *  Created on: Jul 4, 2018
 *      Author: wizath
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define EEPROM_ADDR						0b1010000

/* MODULE EEPROM ADDRESSES */
#define VERSION_EEPROM_ADDRESS			0	/* 1 byte */
#define CAL_DATE_EEPROM_ADDRESS			1	/* 4 bytes 1-4 */

#define DAC1_EEPROM_ADDRESS				16	/* 2 bytes 16-17 */
#define DAC2_EEPROM_ADDRESS				18	/* 2 bytes 18-19 */

#define ADC1_OFFSET_ADDRESS				20	/* 2 bytes 20-21 */
#define ADC2_OFFSET_ADDRESS				22	/* 2 bytes 22-23 */

#define ADC1_SCALE_ADDRESS				24	/* 2 bytes 24-25 */
#define ADC2_SCALE_ADDRESS				26	/* 2 bytes 26-27 */

#define BIAS_DAC_VALUE_ADDRESS			28	/* 2 bytes 28-29 */

#define HW_INT_SCALE					30	/* 4 bytes 30-33 */
#define HW_INT_OFFSET					34	/* 4 bytes 34-37 */

/* MAIN BOARD EEPROM ADDRESSES */
#define IP_METHOD						10
#define MAC_ADDRESS_SELECT				11
#define MAC_ADDRESS						12

#define IP_ADDRESS						18
#define IP_ADDRESS_GW					22
#define IP_ADDRESS_NETMASK				26

void eeprom_write(uint8_t address, uint8_t data);
uint8_t eeprom_read(uint8_t address);
uint16_t eeprom_read16(uint8_t address);
void eeprom_write16(uint8_t address, uint16_t value);
uint32_t eeprom_read32(uint8_t address);
void eeprom_write32(uint8_t address, uint32_t value);

void eeprom_write_mb(uint8_t address, uint8_t data);
uint8_t eeprom_read_mb(uint8_t address);

#endif /* EEPROM_H_ */
