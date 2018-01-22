/*
 * max6639.h
 *
 *  Created on: Nov 21, 2017
 *      Author: wizath
 */

#ifndef MAX6639_H_
#define MAX6639_H_

#include "config.h"

#define MAX6639_I2C								I2C1

#define MAX6639_REG_TEMP(ch)                    (0x00 + (ch))
#define MAX6639_REG_STATUS                      0x02
#define MAX6639_REG_OUTPUT_MASK                 0x03
#define MAX6639_REG_GCONFIG                     0x04
#define MAX6639_REG_TEMP_EXT(ch)                (0x05 + (ch))
#define MAX6639_REG_ALERT_LIMIT(ch)             (0x08 + (ch))
#define MAX6639_REG_OT_LIMIT(ch)                (0x0A + (ch))
#define MAX6639_REG_THERM_LIMIT(ch)             (0x0C + (ch))
#define MAX6639_REG_FAN_CONFIG1(ch)             (0x10 + (ch) * 4)
#define MAX6639_REG_FAN_CONFIG2a(ch)            (0x11 + (ch) * 4)
#define MAX6639_REG_FAN_CONFIG2b(ch)            (0x12 + (ch) * 4)
#define MAX6639_REG_FAN_CONFIG3(ch)             (0x13 + (ch) * 4)
#define MAX6639_REG_FAN_CNT(ch)                 (0x20 + (ch))
#define MAX6639_REG_TARGET_CNT(ch)              (0x22 + (ch))
#define MAX6639_REG_FAN_PPR(ch)                 (0x24 + (ch))
#define MAX6639_REG_FAN_MINTACH(ch)             (0x24 + (ch))
#define MAX6639_REG_TARGTDUTY(ch)               (0x26 + (ch))
#define MAX6639_REG_FAN_START_TEMP(ch)          (0x28 + (ch))
#define MAX6639_REG_DEVID                       0x3D
#define MAX6639_REG_MANUID                      0x3E
#define MAX6639_REG_DEVREV                      0x3F

bool max6639_test(uint8_t addr);
void max6639_init(void);
float max6639_get_temperature(uint8_t addr, uint8_t ch);

#endif /* MAX6639_H_ */
