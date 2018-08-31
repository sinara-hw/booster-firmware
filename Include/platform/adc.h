/*
 * adc.h
 *
 *  Created on: Sep 28, 2017
 *      Author: wizath
 */

#ifndef ADC_H_
#define ADC_H_

#include "config.h"

void adc_init(void);
uint16_t adc_autotest(void);
void prvADCTask(void *pvParameters);

#endif /* ADC_H_ */
