/*
 * usb.h
 *
 *  Created on: Nov 28, 2017
 *      Author: wizath
 */

#ifndef USB_H_
#define USB_H_

#include "config.h"

void USBD_USR_Init(void);
void USBD_USR_DeviceReset(uint8_t speed );
void USBD_USR_DeviceConfigured(void);
void USBD_USR_DeviceSuspended(void);
void USBD_USR_DeviceResumed(void);
void USBD_USR_DeviceConnected(void);
void USBD_USR_DeviceDisconnected(void);
void usb_init(void);

#endif /* USB_H_ */
