/*
 * server.h
 *
 *  Created on: Aug 1, 2017
 *      Author: wizath
 */

#ifndef INC_SERVER_H_
#define INC_SERVER_H_

#include "config.h"

typedef struct {
	uint8_t 	ipsrc[4];
	uint16_t 	ipsrc_port;
} user_data_t;

//static void udp_int_init(void);
//void EXTI15_10_IRQHandler(void);
//static void prvSetupUDPServer(void);
void prvUDPServerTask(void *pvParameters);

#endif /* INC_SERVER_H_ */
