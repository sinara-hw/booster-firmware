/*
 * temp_mgt.h
 *
 *  Created on: Jun 6, 2018
 *      Author: wizath
 */

#ifndef TEMP_MGT_H_
#define TEMP_MGT_H_

void prvTempMgtTask(void *pvParameters);
void set_fan_speed(uint8_t value);
double get_avg_temp(void);
uint8_t get_fan_spped(void);


#endif /* TEMP_MGT_H_ */
