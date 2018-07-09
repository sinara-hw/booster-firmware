/*
 * eeprom.h
 *
 *  Created on: Jul 4, 2018
 *      Author: wizath
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#define EEPROM_ADDR		0b1010000

#define DAC1_EEPROM_ADDRESS		16
#define DAC2_EEPROM_ADDRESS		18
#define ADC1_OFFSET_ADDRESS		20
#define ADC2_OFFSET_ADDRESS		22
#define ADC1_SCALE_ADDRESS		24
#define ADC2_SCALE_ADDRESS		26

void eeprom_write(uint8_t address, uint8_t data);
uint8_t eeprom_read(uint8_t address);

#endif /* EEPROM_H_ */
