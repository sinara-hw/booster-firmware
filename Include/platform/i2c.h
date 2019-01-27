/*
 * i2c.h
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#ifndef I2C_H_
#define I2C_H_

#include "config.h"

#define I2C_TIMEOUT 		256
#define I2C_ACK_ENABLE		1
#define I2C_ACK_DISABLE		0

#define I2C_MUX_ADDR		0x70
#define I2C_DAC_ADDR		0x4C
#define I2C_DUAL_DAC_ADDR	0x0E
#define I2C_MODULE_TEMP		0x4A

void i2c_init(void);
uint8_t i2c_device_connected(I2C_TypeDef* I2Cx, uint8_t address);

// generic read / write
uint8_t i2c_write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data);
uint8_t i2c_read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t * data);

// core functions
uint8_t i2c_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack);
void i2c_stop(I2C_TypeDef* I2Cx);
uint8_t i2c_write_byte(I2C_TypeDef* I2Cx, uint8_t data);
uint8_t i2c_read_byte_ack(I2C_TypeDef* I2Cx, uint8_t * data);
uint8_t i2c_read_byte_nack(I2C_TypeDef* I2Cx, uint8_t * data);
uint8_t i2c_scan_devices(bool verbose);
uint8_t i2c_mux_select(uint8_t channel);

uint32_t i2c_get_channel_errors(uint8_t channel);
void i2c_dac_set(uint16_t value);
void i2c_dual_dac_set(uint8_t ch, uint16_t value);
void i2c_dual_dac_set_val(float v1, float v2);
void i2c_dac_set_value(float value);
void i2c_mux_reset(void);

#endif /* I2C_H_ */
