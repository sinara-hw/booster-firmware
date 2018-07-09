/*
 * temp_mgt.c
 *
 *  Created on: Jun 6, 2018
 *      Author: wizath
 */

#include "config.h"
#include "i2c.h"
#include "channels.h"
#include "max6639.h"
#include "locks.h"

#define TEMP_SENS_I2C_ADDR			0x4A
#define TEMP_SENS_LOCAL_TEMP_L		0x10
#define TEMP_SENS_LOCAL_TEMP_H		0x00
#define TEMP_SENS_REMOTE_TEMP_L		0x11
#define TEMP_SENS_REMOTE_TEMP_H		0x01

#define THRESHOLD 					1.0f
#define MIN_TEMP 					35
#define MAX_TEMP 					80
#define MAX_SPEED 					100
#define MIN_SPEED 					30

static float moving_avg(float temp)
{
    static float fAve = 0.0f;
    static uint16_t fSmp = 0;
    float weight = 0.0f;

    fSmp++;
    weight = 1.0f / fSmp;
    fAve = (weight * temp) + ((1 - weight) * fAve);

    return fAve;
}

static int fan_speed(float temp)
{
	int a = ((MAX_SPEED - MIN_SPEED) / (MAX_TEMP - MIN_SPEED));
	int b = ((MIN_SPEED * MAX_SPEED - MAX_SPEED * MIN_TEMP) / (MAX_TEMP - MIN_TEMP));
	int duty = a * temp + b;

	return duty;
}

extern channel_t channels[8];
volatile uint8_t f_speed = 0;

void set_fan_speed(uint8_t value)
{
	uint8_t address_list[] = {0x2C, 0x2E, 0x2F};
	uint8_t addr = 0;

	if (f_speed != value)
	{
		if (lock_take(I2C_LOCK, portMAX_DELAY))
		{
			for (int i = 0; i < 3; i++)
			{
				addr = address_list[i];
				i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGTDUTY(0), value);
				i2c_write(MAX6639_I2C, addr, MAX6639_REG_TARGTDUTY(1), value);
			}

			lock_free(I2C_LOCK);
		}

		f_speed = value;
	}
}

float channel_get_local_temp(void)
{
	float total = 0.0;
	uint8_t temp = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_LOCAL_TEMP_H);
	uint8_t temp_ext = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_LOCAL_TEMP_L);

	total = (float) temp;
	if (temp_ext & 0x40) total += 0.250;
	if (temp_ext & 0x80) total += 0.500;
	if (temp_ext & 0xC0) total += 0.750;

	return total;
}

float channel_get_remote_temp(void)
{
	float total = 0.0;
	uint8_t temp = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_REMOTE_TEMP_H);
	uint8_t temp_ext = i2c_read(I2C1, TEMP_SENS_I2C_ADDR, TEMP_SENS_REMOTE_TEMP_L);

	total = (float) temp;
	if (temp_ext & 0x40) total += 0.250;
	if (temp_ext & 0x80) total += 0.500;
	if (temp_ext & 0xC0) total += 0.750;

	return total;

//	float min = 30.0f;
//	float max = 60.0f;
//	float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
//	return min + scale * ( max - min );
}

void channel_temp_sens_init(void)
{
	i2c_write(I2C1, I2C_MODULE_TEMP, 0x09, 128);
}

volatile float fTemp, fAvg, chTemp = 0.0f;

void prvTempMgtTask(void *pvParameters)
{
	float th = 0.0f;
	//float chTemp = 0.0f;
	uint8_t speed = 0;
	float maxTemp = 0.0f;

	for (;;)
	{
		for (int i = 0; i < 8; i++)
			if (channels[i].remote_temp > maxTemp) maxTemp = channels[i].remote_temp;

		chTemp = maxTemp;
		fAvg = moving_avg(chTemp);
		th = abs(fTemp - fAvg);

		if (th > THRESHOLD) {
			fTemp = fAvg;

			if (fTemp > MIN_TEMP) {
				speed = fan_speed(fTemp);
				set_fan_speed(speed);
				printf("Temp %f -> Fan %d\n", fTemp, speed);
			} else {
				set_fan_speed(0);
			}
		}

		maxTemp = 0.0;
		vTaskDelay(100);
	}
}
