/*
 * i2c.c
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */


#include "stm32f4xx_gpio.h"
#include "stm32f4xx_i2c.h"
#include "i2c.h"
#include "ucli.h"

typedef enum {
	I2C_TRANSMIT_OK,
	I2C_TRANSMIT_BUSY,
	I2C_TRANSMIT_SELECT_TIMEOUT,
	I2C_TRANSMIT_DIRECION_TIMEOUT,
	I2C_TRANSMIT_TIMEOUT,
} i2c_core_status_t;

typedef enum {
	I2C_OK,
	I2C_ERR
} i2c_status_t;

static uint8_t mux_channel = 0;
static uint32_t i2c_errors[8] = { 0 };

uint32_t i2c_get_channel_errors(uint8_t channel)
{
	if (channel < 8)
		return i2c_errors[channel];

	return 0;
}

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
	I2C_InitStructure.I2C_ClockSpeed = 100000; 			// 100kHz
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;			// I2C mode
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;	// 50% duty cycle --> standard
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;			// own address, not relevant in master mode
	I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;		// disable acknowledge when reading (can be changed later on)
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_Init(I2C1, &I2C_InitStructure);		// init I2C1

	// enable I2C1
	I2C_Cmd(I2C1, ENABLE);

	/* ============================================================ */

	// enable clocks
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C2, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	// configure GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	// Connect I2C1 pins to AF
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_I2C2);	// SCL
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_I2C2); 	// SDA

	// configure I2C1
	I2C_InitStructure.I2C_ClockSpeed = 100000; 				// 100kHz
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;				// I2C mode
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;		// 50% duty cycle --> standard
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;				// own address, not relevant in master mode
	I2C_InitStructure.I2C_Ack = I2C_Ack_Disable;			// disable acknowledge when reading (can be changed later on)
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit; // set address length to 7 bit addresses
	I2C_InitStructure.I2C_OwnAddress1 = 0x00;
	I2C_Init(I2C2, &I2C_InitStructure);						// init I2C1

	// enable I2C1
	I2C_Cmd(I2C2, ENABLE);
}

void i2c_reset(void)
{
	// due to shorting SCL pin to ground can cause glitches
	// that are gone only after rebooting whole interface
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, DISABLE);
	vTaskDelay(10);
	i2c_init();
}

uint8_t i2c_mux_select(uint8_t channel)
{
	if (channel > 7) return 2;

	i2c_mux_reset();

	if (!i2c_start(I2C1, I2C_MUX_ADDR, I2C_Direction_Transmitter, 0))
	{
		i2c_write_byte(I2C1, (1 << channel));
		i2c_stop(I2C1);
		mux_channel = channel;

		return 0;
	}

	return 1;
}

void i2c_mux_reset(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_14);
	for (int i = 0; i < 256; i++){};
	GPIO_SetBits(GPIOB, GPIO_Pin_14);
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

uint8_t i2c_write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data)
{
	if (i2c_start(I2Cx, address, I2C_Direction_Transmitter, I2C_ACK_DISABLE))
		return I2C_ERR;

	if (i2c_write_byte(I2Cx, reg))
		return I2C_ERR;

	if (i2c_write_byte(I2Cx, data))
		return I2C_ERR;

	i2c_stop(I2Cx);
	return I2C_OK;
}

uint8_t i2c_read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t * data)
{
	if (i2c_start(I2Cx, address, I2C_Direction_Transmitter, I2C_ACK_DISABLE))
		return I2C_ERR;

	if (i2c_write_byte(I2Cx, reg))
		return I2C_ERR;

	i2c_stop(I2Cx);

	if (i2c_start(I2Cx, address, I2C_Direction_Receiver, I2C_ACK_DISABLE))
		return I2C_ERR;

	uint8_t received_data;
	if (i2c_read_byte_nack(I2Cx, &received_data))
		return I2C_ERR;

	i2c_stop(I2Cx);
	*data = received_data;

	return received_data;
}


uint8_t i2c_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack)
{
	uint32_t timeout = I2C_TIMEOUT;

	// wait until I2C1 is not busy anymore
	while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY)) {
		if (--timeout == 0x00) {
			i2c_errors[mux_channel]++;
			rf_channel_disable_on_error(mux_channel);
			ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_BUSY error\r\n");
			return I2C_TRANSMIT_BUSY;
		}
	}

	I2C_AcknowledgeConfig(I2C1, ack ? ENABLE : DISABLE);

	// Send I2C1 START condition
	I2C_GenerateSTART(I2Cx, ENABLE);

	// wait for I2C1 EV5 --> Slave has acknowledged start condition
	timeout = I2C_TIMEOUT;
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_MODE_SELECT)) {
		if (--timeout == 0x00) {
			i2c_errors[mux_channel]++;
			rf_channel_disable_on_error(mux_channel);
			ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_SELECT_TIMEOUT error\r\n");
			return I2C_TRANSMIT_SELECT_TIMEOUT;
		}
	}

	// Send slave Address for write
	I2C_Send7bitAddress(I2Cx, (address << 1), direction);

	if (direction == I2C_Direction_Transmitter) {
		timeout = I2C_TIMEOUT;
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) {
			if (--timeout == 0x00) {
				i2c_errors[mux_channel]++;
				rf_channel_disable_on_error(mux_channel);
				ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_DIRECION_TIMEOUT error\r\n");
				return I2C_TRANSMIT_DIRECION_TIMEOUT;
			}
		}
	} else if (direction == I2C_Direction_Receiver) {
		timeout = I2C_TIMEOUT;
		while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) {
			if (--timeout == 0x00) {
				i2c_errors[mux_channel]++;
				rf_channel_disable_on_error(mux_channel);
				ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_DIRECION_TIMEOUT error\r\n");
				return I2C_TRANSMIT_DIRECION_TIMEOUT;
			}
		}
	}

	return I2C_TRANSMIT_OK;
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
			i2c_errors[mux_channel]++;
			ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_TIMEOUT error\r\n");
			return I2C_TRANSMIT_TIMEOUT;
		}
	}

	return I2C_TRANSMIT_OK;
}

uint8_t i2c_read_byte_ack(I2C_TypeDef* I2Cx, uint8_t * data)
{
	uint32_t timeout = I2C_TIMEOUT;

	I2C_AcknowledgeConfig(I2Cx, ENABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
		if (--timeout == 0x00) {
			i2c_errors[mux_channel]++;
			rf_channel_disable_on_error(mux_channel);
			ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_TIMEOUT error\r\n");
			return I2C_TRANSMIT_TIMEOUT;
		}
	}

	*data = I2C_ReceiveData(I2Cx);
	return I2C_TRANSMIT_OK;
}

uint8_t i2c_read_byte_nack(I2C_TypeDef* I2Cx, uint8_t * data)
{
	uint32_t timeout = I2C_TIMEOUT;

	I2C_AcknowledgeConfig(I2Cx, DISABLE);
	while (!I2C_CheckEvent(I2Cx, I2C_EVENT_MASTER_BYTE_RECEIVED)) {
		if (--timeout == 0x00) {
			i2c_errors[mux_channel]++;
			rf_channel_disable_on_error(mux_channel);
			ucli_log(UCLI_LOG_ERROR, "I2C_TRANSMIT_TIMEOUT error\r\n");
			return I2C_TRANSMIT_TIMEOUT;
		}
	}

	*data = I2C_ReceiveData(I2Cx);
	return I2C_TRANSMIT_OK;
}

void i2c_dac_set(uint16_t value)
{
	uint8_t first_byte = (value & 0xFF00) >> 8;
	uint8_t second_byte = value & 0xFF;

	uint8_t a1 = i2c_start(I2C1, I2C_DAC_ADDR, I2C_Direction_Transmitter, 1);
	uint8_t a2 = i2c_write_byte(I2C1, first_byte);
	uint8_t a3 = i2c_write_byte(I2C1, second_byte);
	i2c_stop(I2C1);
}

void i2c_dac_set_value(float value)
{
	float div_value = (((value - 1.22f) * 22000) / 10000) - 1.226f;
	uint16_t dac_value = (div_value * 4095) / 2.5f;

	uint8_t first_byte = (dac_value & 0xFF00) >> 8;
	uint8_t second_byte = dac_value & 0xFF;

	i2c_start(I2C1, I2C_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, first_byte);
	i2c_write_byte(I2C1, second_byte);
	i2c_stop(I2C1);
}

void i2c_dual_dac_set(uint8_t ch, uint16_t value)
{
	// enable internal reference
	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 127);
	i2c_write_byte(I2C1, 0);
	i2c_write_byte(I2C1, 1);
	i2c_stop(I2C1);

	uint16_t regval = value & 0xFFF;

	if (ch == 0) {
		i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
		i2c_write_byte(I2C1, 0b01011000);

		i2c_write_byte(I2C1, (regval & 0xFF0) >> 4);
		i2c_write_byte(I2C1, (regval & 0x00F) << 4);

		i2c_stop(I2C1);
	}

	if (ch == 1) {
		i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
		i2c_write_byte(I2C1, 0b01011001);

		i2c_write_byte(I2C1, (regval & 0xFF0) >> 4);
		i2c_write_byte(I2C1, (regval & 0x00F) << 4);

		i2c_stop(I2C1);
	}
}

void i2c_dual_dac_set_val(float v1, float v2)
{
	uint16_t value1 = (uint16_t) ((float) ((v1 * 4095.0f) / ( 2 * 2.2f )));
	uint16_t value2 = (uint16_t) ((float) ((v2 * 4095.0f) / ( 2 * 2.2f )));

	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 127);
	i2c_write_byte(I2C1, 0);
	i2c_write_byte(I2C1, 1);
	i2c_stop(I2C1);

	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 0b01011000);
	i2c_write_byte(I2C1, (value1 & 0xFF0) >> 4);
	i2c_write_byte(I2C1, (value1 & 0x00F) << 4);
	i2c_stop(I2C1);

	i2c_start(I2C1, I2C_DUAL_DAC_ADDR, I2C_Direction_Transmitter, 1);
	i2c_write_byte(I2C1, 0b01011001);
	i2c_write_byte(I2C1, (value2 & 0xFF0) >> 4);
	i2c_write_byte(I2C1, (value2 & 0x00F) << 4);
	i2c_stop(I2C1);
}
