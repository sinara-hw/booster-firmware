/*
 * max6642.h
 *
 *  Created on: Jul 12, 2018
 *      Author: wizath
 */

#ifndef DEVICES_MAX6642_H_
#define DEVICES_MAX6642_H_

#include "config.h"

void max6642_init(void);
float max6642_get_local_temp(void);
float max6642_get_remote_temp(void);

#endif /* DEVICES_MAX6642_H_ */
