/*
 * tasks.h
 *
 *  Created on: Jul 12, 2018
 *      Author: wizath
 */

#ifndef TASKS_H_
#define TASKS_H_

#include "config.h"

extern TaskHandle_t task_rf_measure;
extern TaskHandle_t task_rf_info;
extern TaskHandle_t task_rf_interlock;
extern TaskHandle_t xDHCPTask;
extern TaskHandle_t xUDPServerTask;

#endif /* TASKS_H_ */
