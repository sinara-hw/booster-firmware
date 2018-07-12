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
void channel_temp_sens_init(void);
float channel_get_local_temp(void);
float channel_get_remote_temp(void);

#endif /* TEMP_MGT_H_ */
