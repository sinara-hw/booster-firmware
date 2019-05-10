/*
 * led_bar.h
 *
 *  Created on: Nov 28, 2017
 *      Author: wizath
 */

#ifndef LED_BAR_H_
#define LED_BAR_H_

#include "config.h"

void led_bar_write(uint8_t green, uint8_t yellow, uint8_t red);
void led_bar_init();

#endif /* LED_BAR_H_ */
