/*
 * eeprom.h
 *
 *  Created on: Jul 4, 2018
 *      Author: wizath
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define EEPROM_ADDR						0b1010000

#define SOFT_INTERLOCK_ADDRESS  		14

#define DAC1_EEPROM_ADDRESS				16
#define DAC2_EEPROM_ADDRESS				18

#define ADC1_OFFSET_ADDRESS				20
#define ADC2_OFFSET_ADDRESS				22

#define ADC1_SCALE_ADDRESS				24
#define ADC2_SCALE_ADDRESS				26

#define BIAS_DAC_VALUE_ADDRESS			28

#define HW_INT_SCALE					30
#define HW_INT_OFFSET					34

void eeprom_write(uint8_t address, uint8_t data);
uint8_t eeprom_read(uint8_t address);
uint16_t eeprom_read16(uint8_t address);
void eeprom_write16(uint8_t address, uint16_t value);
uint32_t eeprom_read32(uint8_t address);
void eeprom_write32(uint8_t address, uint32_t value);

#endif /* EEPROM_H_ */
