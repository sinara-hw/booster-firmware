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
double temp_mgt_get_avg_temp(void);
uint8_t temp_mgt_get_fanspeed(void);
double temp_mgt_get_max_temp(void);

#endif /* TEMP_MGT_H_ */
