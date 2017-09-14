/*
 * i2c.h
 *
 *  Created on: Sep 14, 2017
 *      Author: rst
 */

#ifndef I2C_H_
#define I2C_H_

#include "config.h"

void init_i2c(void);
uint8_t i2c_device_connected(I2C_TypeDef* I2Cx, uint8_t address);
void i2c_write(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg, uint8_t data);
uint8_t i2c_read(I2C_TypeDef* I2Cx, uint8_t address, uint8_t reg);
uint8_t i2c_start(I2C_TypeDef* I2Cx, uint8_t address, uint8_t direction, uint8_t ack);
void i2c_stop(I2C_TypeDef* I2Cx);
uint8_t i2c_write_byte(I2C_TypeDef* I2Cx, uint8_t data);
uint8_t i2c_read_byte_ack(I2C_TypeDef* I2Cx);
uint8_t i2c_read_byte_nack(I2C_TypeDef* I2Cx);
uint8_t i2c_scan_devices(void);
uint8_t i2c_mux_select(uint8_t channel);

#endif /* I2C_H_ */
