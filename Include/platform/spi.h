/*
 * spi.h
 *
 *  Created on: Sep 18, 2017
 *      Author: rst
 */

#ifndef SPI_H_
#define SPI_H_

#include "config.h"

void spi_init(void);
uint8_t spi_send_byte(uint8_t byte);
uint8_t spi_receive_byte(void);
void spi_dma_read(uint8_t* Addref, uint8_t* pRxBuf, uint16_t rx_len);
void spi_dma_write(uint8_t* Addref, uint8_t* pTxBuf, uint16_t tx_len);

#endif /* SPI_H_ */
