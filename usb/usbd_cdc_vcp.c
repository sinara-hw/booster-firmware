/**
  ******************************************************************************
  * @file    usbd_cdc_vcp.c
  * @author  MCD Application Team
  * @version V1.2.0
  * @date    09-November-2015
  * @brief   Generic media access Layer.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED 
#pragma     data_alignment = 4 
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "usbd_cdc_vcp.h"
#include "stm32f4xx.h"
#include "stm32f4xx_gpio.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint8_t rx_buffer[64] = { 0 };
extern USB_OTG_CORE_HANDLE USB_OTG_dev;
extern xQueueHandle _xRxQueue;

/* Private function prototypes -----------------------------------------------*/
static uint16_t VCP_DataTx   (void);
static uint16_t VCP_DataRx   (uint32_t Len);

CDC_IF_Prop_TypeDef VCP_fops = 
{
  VCP_DataTx,
  VCP_DataRx
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  VCP_DataTx
  *         CDC received data to be send over USB IN endpoint are managed in 
  *         this function.
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */
static uint16_t VCP_DataTx (void)
{
	return USBD_OK;
}

/**
  * @brief  VCP_DataRx
  *         Data received over USB OUT endpoint are sent over CDC interface 
  *         through this function.
  *           
  *         @note
  *         This function will block any OUT packet reception on USB endpoint 
  *         until exiting this function. If you exit this function before transfer
  *         is complete on CDC interface (ie. using DMA controller) it will result 
  *         in receiving more data while previous ones are still not sent.
  *                 
  * @param  Len: Number of data received (in bytes)
  * @retval Result of the operation: USBD_OK if all operations are OK else VCP_FAIL
  */
static uint16_t VCP_DataRx (uint32_t Len)
{
	VCP_ReceiveData(&USB_OTG_dev, rx_buffer, Len);

	for (int i = 0; i < Len; i++)
	{
		char ch = (char) rx_buffer[i];
		xQueueSend(_xRxQueue, &ch, 0);
	}

	return USBD_OK;
}

/* send data function */
void VCP_SendData( USB_OTG_CORE_HANDLE *pdev, uint8_t* pbuf, uint32_t  buf_len)
{
	DCD_EP_Tx(pdev, CDC_IN_EP, pbuf , buf_len );
}

/* prepare data to be received */
void VCP_ReceiveData(USB_OTG_CORE_HANDLE *pdev, uint8_t  *pbuf, uint32_t   buf_len)
{
    DCD_EP_PrepareRx(pdev, CDC_OUT_EP, pbuf, buf_len);
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
