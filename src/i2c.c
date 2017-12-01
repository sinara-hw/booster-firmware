/*
 * i2c.c
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */


#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "i2c.h"

#define I2C_TIMEOUT 		200
#define I2C_ACK_ENABLE		1
#define I2C_ACK_DISABLE		0
#define I2C_MUX_ADDR		0x70

void i2c_init(void)
{
	I2C_InitTypeDef  I2C_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;

	// enable clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	// configure GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Connect I2C1 pins to AF
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_I2C1);	// SCL
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_I2C1); // SDA

	// configure I2C1
	I2C_InitStructure.I2C_ClockSpeed = 1000000; 			// 100kHz
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;			// I2C mode
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;	// 50% duty cycle --> standard
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;			// own address, not relevant in master mode
	I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;		// disable acknowledge when reading (can be changed later on)
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_Init(I2C1, &I2C_InitStructure);		// init I2C1

	// enable I2C1
	I2C_Cmd(I2C1, ENABLE);
}

uint8_t i2c_mux_select(uint8_t channel)
{
	if (channel > 7) return 2;

	if (!i2c_start(I2C1, I2C_MUX_ADDR, I2C_Direction_Transmitter, 0))
	{
		i2c_write_byte(I2C1, (1 << channel));
		i2c_stop(I2C1);

		return 0;
	}

	return 1;
}

uint8_t i2c_scan_devices(bool verbose)
{
	uint8_t connected = 0;

	if (verbose) printf("[i2c_scan] start\n");

	for (int addr = 0; addr < 128; addr++)
	{
		if (i2c_device_connected(I2C1, addr))
		{
			if (verbose) printf("[i2c_scan] Found I2C device at %X\n", addr);
			connected++;
		}
	}

	if (verbose) printf("[i2c_scan] end\n");

	return connected;
}

uint8_t i2c_device_connected(I2C_TypeDef* I2Cx, uint8_t address)
{
	uint8_t connected = 0;
	if (!i2c_start(I2Cx, address, I2C_Direction_Transmitter, I2C_ACK_ENABLE)) {
		connected = 1;
	}

	i2c_stop(I2Cx);

	return connected;
}


void i2c_write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data)
{
	i2c_start(I2Cx, address, I2C_Direction_Transmitter, I2C_ACK_DISABLE);
	i2c_write_byte(I2Cx, reg);
	i2c_write_byte(I2Cx, data);
	i2c_stop(I2Cx);
}


uint8_t i2c_read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg)
{
	uint8_t received_data;
	i2c_start(I2Cx, address, I2C_Direction_Transmitter, I2C_ACK_DISABLE);
	i2c_write_byte(I2Cx, reg);
	i2c_stop(I2Cx);

	i2c_start(I2Cx, address, I2C_Direction_Receiver, I2C_ACK_DISABLE);
	received_data = i2c_read_byte_nack(I2Cx);
	i2c_stop(I2Cx);

	return received_data;
}


uint8_t i2c_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack)
{
	uint32_t timeout = I2C_TIMEOUT;

	// wait until I2C1 is not busy anymore
	while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY)) {
		if (--timeout == 0x00) {
			return 1;
		}
	}

	I2C_AcknowledgeConfig(I2C1, ack ? ENABLE : DISABLE);

	// Send I2C1 START condition
	I2C_GenerateSTART(I2Cx, ENABLE);

	// wait for I2C1 EV5 --> Slave has acknowledged start condition
	timeout = I2C_TIMEOUT;
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
		if (--timeout == 0x00) {
			return 1;
		}
	}

	// Send slave Address for write
	I2C_Send7bitAddress(I2Cx, (address << 1), direction);

	if (direction == I2C_Direction_Transmitter) {
		timeout = I2C_TIMEOUT;
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (--timeout == 0x00) {
				return 1;
			}
		}
	} else if (direction == I2C_Direction_Receiver) {
		timeout = I2C_TIMEOUT;
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (--timeout == 0x00) {
				return 1;
			}
		}
	}

	return 0;
}


void i2c_stop(I2C_TypeDef* I2Cx)
{
	I2C_GenerateSTOP(I2Cx, ENABLE);
}


uint8_t i2c_write_byte(I2C_TypeDef* I2Cx, uint8_t data)
{
	uint32_t timeout = I2C_TIMEOUT;

	I2C_SendData(I2Cx, data);

	// wait for I2C1 EV8_2 --> byte has been transmitted
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_TRANSMITTED)) {
		if (--timeout == 0x00) {
			return 1;
		}
	}

	return 0;
}

uint8_t i2c_read_byte_ack(I2C_TypeDef* I2Cx)
{
	uint32_t timeout = I2C_TIMEOUT;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) && --timeout);

	return I2C_ReceiveData(I2Cx);
}

uint8_t i2c_read_byte_nack(I2C_TypeDef* I2Cx)
{
	uint32_t timeout = I2C_TIMEOUT;

	I2C_AcknowledgeConfig(I2Cx, DISABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED) && --timeout);

	return I2C_ReceiveData(I2Cx);
}
