/*
 * ad5627.h
 *
 *  Created on: Jul 13, 2018
 *      Author: wizath
 */

#ifndef DEVICES_AD5627_H_
#define DEVICES_AD5627_H_

void ad5627_init(void);
void ad5627_set_value(uint8_t ch, uint16_t value);
void ad5627_set_voltage(float v1, float v2);

#endif /* DEVICES_AD5627_H_ */
