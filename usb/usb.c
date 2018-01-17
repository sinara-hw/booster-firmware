/*
 * usb.c
 *
 *  Created on: Nov 28, 2017
 *      Author: wizath
 */

#include "usb.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"
#include "usb_dcd_int.h"

__ALIGN_BEGIN USB_OTG_CORE_HANDLE    USB_OTG_dev __ALIGN_END;

#define RX_QUEUE_SIZE	512
xQueueHandle			_xRxQueue;

uint8_t bootlog[4096] = { 0 };
uint8_t log_cnt = 0;
bool print_enable = false;

int __io_putchar(int ch)
{
#ifdef DEBUG
	taskENTER_CRITICAL();
	/* Place your implementation of fputc here */
	/* e.g. write a character to the USART */
//	if (print_enable){
	VCP_SendData(&USB_OTG_dev, (uint8_t *) &ch, 1);
//	} else {
//		if (log_cnt < 4095) bootlog[log_cnt++] = ch;
//	}

	taskEXIT_CRITICAL();

	return ch;
#endif
}

void usb_bootlog(void)
{
	puts(bootlog);
}

USBD_Usr_cb_TypeDef USR_cb =
{
  USBD_USR_Init,
  USBD_USR_DeviceReset,
  USBD_USR_DeviceConfigured,
  USBD_USR_DeviceSuspended,
  USBD_USR_DeviceResumed,


  USBD_USR_DeviceConnected,
  USBD_USR_DeviceDisconnected,
};

void usb_init(void)
{
	_xRxQueue = xQueueCreate(RX_QUEUE_SIZE, sizeof(uint16_t));
	USBD_Init(&USB_OTG_dev, USB_OTG_FS_CORE_ID, &USR_desc, &USBD_CDC_cb, &USR_cb);
}

void USBD_USR_Init(void)
{
}

void USBD_USR_DeviceReset(uint8_t speed )
{
}

void USBD_USR_DeviceConfigured (void)
{
}

void USBD_USR_DeviceSuspended(void)
{
}

void USBD_USR_DeviceResumed(void)
{
}

void USBD_USR_DeviceConnected (void)
{
}

void USBD_USR_DeviceDisconnected (void)
{
}

#ifdef USE_USB_OTG_FS
void OTG_FS_WKUP_IRQHandler(void)
{
	if (USB_OTG_dev.cfg.low_power)
	{
		*(uint32_t *)(0xE000ED10) &= 0xFFFFFFF9 ;
		SystemInit();
		USB_OTG_UngateClock(&USB_OTG_dev);
	}
	EXTI_ClearITPendingBit(EXTI_Line18);
}
#endif

void OTG_FS_IRQHandler(void)
{
	USBD_OTG_ISR_Handler (&USB_OTG_dev);
}
