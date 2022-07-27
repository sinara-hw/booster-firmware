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
#include "ucli.h"
#include "led_bar.h"
#include "device.h"

static volatile double fTemp, fAvg, chTemp = 0.0f;
static volatile uint8_t f_speed = 0;

static double moving_avg(float temp)
{
    static double fAve = 0.0f;
    static uint16_t fSmp = 0;
    double weight = 0.0f;

    fSmp++;
    weight = 4.0f / fSmp;
    fAve = (weight * (double) temp) + ((1 - weight) * fAve);

    return fAve;
}

// linear coefficients
// 32 degree -> 20% FAN
// 50 degree -> 50% FAN
// 68 degree -> 80% FAN

#define MIN_TEMP 					40
#define MAX_TEMP 					60
#define MAX_SPEED 					100
#define MIN_SPEED 					0

static int fan_speed(float temp, uint8_t min_speed, uint8_t threshold)
{
	float a = ((MAX_SPEED - MIN_SPEED) / (MAX_TEMP - MIN_TEMP));
	float b = ((MIN_SPEED * MAX_SPEED - MAX_SPEED * MIN_TEMP) / (MAX_TEMP - MIN_TEMP));
	float duty = a * temp + b;

	if (duty < min_speed) {
		duty = min_speed;
	} else if (duty > MAX_SPEED) {
		duty = MAX_SPEED;
	}

	// cut off fans on selected threshold
	// to avoid 'clicking' fans
	if (duty <= threshold)
		duty = 0;

	return (int) duty;
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

void fan_speed_control(float temp)
{
	uint8_t min_fan_speed = 0;
	device_t * dev = device_get_config();

	if (rf_channels_read_enabled() > 0)
		min_fan_speed = dev->fan_level;
	else
		min_fan_speed = 0;

	uint8_t duty_cycle = fan_speed(temp, min_fan_speed, 15);
	uint8_t fan_delta = abs(duty_cycle - f_speed);
//	printf("Current %d Fan duty %d delta %d temp avg %.2f\r\n", f_speed, duty_cycle, fan_delta, temp);

	if (fan_delta > 5) {
		set_fan_speed(duty_cycle);
		ucli_log(UCLI_LOG_INFO, "[tempmgt] Temp %f (d %d) -> Fan %d\n", temp, fan_delta, duty_cycle);
	}
}

double temp_mgt_get_avg_temp(void)
{
	return fAvg;
}

double temp_mgt_get_max_temp(void)
{
	return chTemp;
}

uint8_t temp_mgt_get_fanspeed(void)
{
	return f_speed;
}

void prvTempMgtTask(void *pvParameters)
{
	device_t * dev = device_get_config();
	double maxTemp = 0.0f;
	fTemp = (MIN_TEMP - 2);

	if (dev->powercfg)
	{
		// auto enable procedure after power-up
		vTaskDelay(2000);
		rf_channels_enable(rf_channels_get_mask());

        // clear interlocks after power-up (it is necessary due to hardware changes in channels v1.6)
        vTaskDelay(100);
        rf_clear_interlock();
        vTaskDelay(100);
        rf_clear_interlock();

		// ensure that channel LED's are light on
		vTaskDelay(100);
		led_bar_or(rf_channels_read_sigon(), 0, 0);
	}

	for (;;)
	{
		for (int i = 0; i < 8; i++) {
			channel_t * ch = rf_channel_get(i);
			if (ch->enabled) {
				if (ch->measure.remote_temp > maxTemp)
					maxTemp = ch->measure.remote_temp;
			}

			vTaskDelay(10);
		}

		chTemp = maxTemp;
		fAvg = moving_avg(maxTemp);

		// guard for NaN values
		if (isnan(fAvg)) fAvg = maxTemp;

		fan_speed_control(fAvg);

		maxTemp = 0.0f;
		vTaskDelay(100);
	}
}
