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
#include "math.h"

#define THRESHOLD 					1.0f
#define MIN_TEMP 					35
#define MAX_TEMP 					80
#define MAX_SPEED 					100
#define MIN_SPEED 					30

static volatile double fTemp, fAvg, chTemp = 0.0f;
static volatile uint8_t f_speed = 0;

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

double get_avg_temp(void)
{
	return fAvg;
}

uint8_t get_fan_spped(void)
{
	return f_speed;
}

void prvTempMgtTask(void *pvParameters)
{
	float th = 0.0f;
	//float chTemp = 0.0f;
	uint8_t speed = 0;
	float maxTemp = 0.0f;

	for (;;)
	{
		for (int i = 0; i < 8; i++) {
			channel_t * ch = rf_channel_get(i);

			if (ch->measure.remote_temp > maxTemp)
				maxTemp = ch->measure.remote_temp;
		}

		chTemp = maxTemp;
		fAvg = moving_avg(chTemp);
		th = abs(fTemp - fAvg);

		// guard for NaN values
		if (isnan(fAvg)) fAvg = 0.0f;

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
