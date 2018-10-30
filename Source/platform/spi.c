/*
 * spi.c
 *
 *  Created on: Sep 18, 2017
 *      Author: rst
 */

#include "spi.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_spi.h"

#define SPI_TIMEOUT 2000

#define DATA_SIZE 	128
DMA_InitTypeDef 	DMA_InitStructure;
uint8_t 			pTmpBuf1[DATA_SIZE + 3];
uint8_t 			pTmpBuf2[DATA_SIZE + 3];

void spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;
//	NVIC_InitTypeDef NVIC_InitStructure;

	// Clocks init
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	// GPIO Init
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(BOARD_WIZNET_CS);

	//WIZNET RST --------------------------

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOG, &GPIO_InitStructure);
	GPIO_SetBits(BOARD_WIZNET_RST);

	//--------------------------------------

	SPI_I2S_DeInit(SPI1);
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
}

uint8_t spi_send_byte(uint8_t byte)
{
	uint32_t timeout = SPI_TIMEOUT;

	SPI_I2S_SendData(SPI1, byte);
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {
		if (--timeout == 0x00) return 66;
	}

	return SPI_I2S_ReceiveData(SPI1);
}

uint8_t spi_receive_byte(void)
{
	uint32_t timeout = SPI_TIMEOUT;

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY) == SET) {
		if (--timeout == 0x00) return 66;
	}

	SPI_I2S_SendData(SPI1, 0);

	timeout = SPI_TIMEOUT;
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET) {
		if (--timeout == 0x00) return 99;
	}

	return SPI_I2S_ReceiveData(SPI1);
}

void spi_dma_read(uint8_t* Addref, uint8_t* pRxBuf, uint16_t rx_len)
{
	memset(pTmpBuf1, 0, rx_len + 3);
	memset(pTmpBuf2, 0, rx_len + 3);

	pTmpBuf1[0] = Addref[0];
	pTmpBuf1[1] = Addref[1];
	pTmpBuf1[2] = Addref[2];

	DMA_InitStructure.DMA_BufferSize = (uint16_t)(rx_len + 3);
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;

	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;

	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

	/* Configure Tx DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) pTmpBuf1;
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);

	/* Configure Rx DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) pTmpBuf2;
	DMA_Init(DMA2_Stream2, &DMA_InitStructure);

	/* Enable the SPI Rx DMA request */
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

	GPIO_ResetBits(BOARD_WIZNET_CS);

	/* Enable the DMA SPI TX Stream */
	DMA_Cmd(DMA2_Stream3, ENABLE);

	/* Enable the DMA SPI RX Stream */
	DMA_Cmd(DMA2_Stream2, ENABLE);

	/* Waiting the end of Data transfer */
	while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET);
	while (DMA_GetFlagStatus(DMA2_Stream2, DMA_FLAG_TCIF2) == RESET);
//	xSemaphoreTake( xDMATxComplete, tcpLONG_DELAY );
//	xSemaphoreTake( xDMARxComplete, tcpLONG_DELAY );

	GPIO_SetBits(BOARD_WIZNET_CS);

	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
	DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);

	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_Cmd(DMA2_Stream2, DISABLE);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);

	memcpy(pRxBuf, pTmpBuf2 + 3, rx_len);
}

void spi_dma_write(uint8_t* Addref, uint8_t* pTxBuf, uint16_t tx_len)
{
	memset(pTmpBuf1, 0, tx_len + 3);

	pTmpBuf1[0] = Addref[0];
	pTmpBuf1[1] = Addref[1];
	pTmpBuf1[2] = Addref[2];

	for (int i = 0; i < tx_len; i++)
		pTmpBuf1[3 + i] = pTxBuf[i];

	DMA_InitStructure.DMA_BufferSize = (uint16_t)(tx_len + 3);
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull;

	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;

	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&(SPI1->DR));
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

	/* Configure Tx DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) pTmpBuf1;
	DMA_Init(DMA2_Stream3, &DMA_InitStructure);

	/* Configure Rx DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_3;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) pTmpBuf1;
	DMA_Init(DMA2_Stream2, &DMA_InitStructure);

	/* Enable the DMA SPI TX Stream */
	DMA_Cmd(DMA2_Stream3, ENABLE);
	/* Enable the DMA SPI RX Stream */
	DMA_Cmd(DMA2_Stream2, ENABLE);

	GPIO_ResetBits(BOARD_WIZNET_CS);

	/* Enable the SPI Rx DMA request */
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, ENABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

	/* Waiting the end of Data transfer */
	while (DMA_GetFlagStatus(DMA2_Stream3, DMA_FLAG_TCIF3) == RESET);
	while (DMA_GetFlagStatus(DMA2_Stream2, DMA_FLAG_TCIF2) == RESET);
//	xSemaphoreTake( xDMATxComplete, tcpLONG_DELAY );
//	xSemaphoreTake( xDMARxComplete, tcpLONG_DELAY );

	GPIO_SetBits(BOARD_WIZNET_CS);

	DMA_ClearFlag(DMA2_Stream3, DMA_FLAG_TCIF3);
	DMA_ClearFlag(DMA2_Stream2, DMA_FLAG_TCIF2);

	DMA_Cmd(DMA2_Stream3, DISABLE);
	DMA_Cmd(DMA2_Stream2, DISABLE);

	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Rx, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
}

