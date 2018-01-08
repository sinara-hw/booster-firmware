/*
 * led_bar.c
 *
 *  Created on: Nov 28, 2017
 *      Author: wizath
 */

#include "stm32f4xx_spi.h"

void led_bar_write(uint8_t green, uint8_t yellow, uint8_t red)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_12);

	SPI_I2S_SendData(SPI2, green);
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);

	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);

	SPI_I2S_SendData(SPI2, yellow);
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);

	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);

	SPI_I2S_SendData(SPI2, red);
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_BSY) == SET);

	GPIO_SetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
}

void led_bar_init()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	SPI_InitTypeDef SPI_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	GPIO_ResetBits(GPIOB, GPIO_Pin_12);
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);

	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

	SPI_I2S_DeInit(SPI2);
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64;
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;

	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2, ENABLE);

	// reset OE pin
	GPIO_SetBits(GPIOB, GPIO_Pin_8);
	GPIO_ResetBits(GPIOB, GPIO_Pin_8);

	led_bar_write(0, 0, 0);
}
