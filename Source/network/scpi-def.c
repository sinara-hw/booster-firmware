/*-
 * Copyright (c) 2012-2013 Jan Breuer,
 *
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file   scpi-def.c
 * @date   Thu Nov 15 10:58:45 UTC 2012
 *
 * @brief  SCPI parser test
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "math.h"
#include "scpi/scpi.h"
#include "scpi-def.h"

#include "adc.h"
#include "channels.h"
#include "led_bar.h"
#include "locks.h"
#include "i2c.h"
#include "eeprom.h"

/**
 * Reimplement IEEE488.2 *TST?
 *
 * Result should be 0 if everything is ok
 * Result should be 1 if something goes wrong
 *
 * Return SCPI_RES_OK
 */
static scpi_result_t My_CoreTstQ(scpi_t * context) {

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

    SCPI_ResultInt32(context, 0);

    return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Enable(scpi_t * context)
{
	uint32_t channel;
	char text[3] = { 0 };
	uint8_t ch_mask = rf_channels_get_mask();

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
		printf("No channel\n");
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	printf("GOT %d\r\n", channel);

	if (channel == ch_mask) {
		rf_channels_enable(channel);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		rf_channels_enable(1 << channel);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Disable(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		rf_channels_disable(channel);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		rf_channels_disable(1 << channel);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t INTERLOCK_Power(scpi_t * context)
{
	uint32_t channel;
	double interlock;
	channel_t * ch;

	if (!SCPI_ParamUInt32(context, &channel, true)) {
		return SCPI_RES_ERR;
	}

	if (!SCPI_ParamDouble(context, &interlock, true)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel < 8) {
		if (interlock >= 0 && interlock <= 38.0) {
			ch = rf_channel_get(channel);
			uint16_t dac_value = (uint16_t) (exp((interlock - ch->cal_values.hw_int_offset) / ch->cal_values.hw_int_scale));
			printf("Calculated value for pwr %0.2f = %d\n", interlock, dac_value);
			ch->cal_values.output_dac_cal_value = dac_value;

			if (lock_take(I2C_LOCK, portMAX_DELAY))
			{
				eeprom_write16(DAC2_EEPROM_ADDRESS, dac_value);
				if (ch->enabled) {
					i2c_mux_select(channel);
					i2c_dual_dac_set(1, ch->cal_values.output_dac_cal_value);
				}
				lock_free(I2C_LOCK);
			}
			return SCPI_RES_OK;
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t INTERLOCK_PowerQ(scpi_t * context)
{
	uint32_t channel;
	channel_t * ch;

	if (!SCPI_ParamUInt32(context, &channel, true)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		double value = log(ch->cal_values.output_dac_cal_value * ch->cal_values.hw_int_scale) + ch->cal_values.hw_int_offset;
		SCPI_ResultDouble(context, value);
		return SCPI_RES_OK;

	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_EnableQ(scpi_t * context)
{
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	SCPI_ResultUInt8(context, rf_channels_read_enabled());

	return SCPI_RES_OK;
}

static scpi_result_t MEASURE_FanQ(scpi_t * context)
{
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	SCPI_ResultUInt8(context, temp_mgt_get_fanspeed());

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Current(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	double data[8] = { 0 };

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->enabled) {
				data[i] = ch->measure.i30;
			} else {
				data[i] = 0;
			}
		}

		SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		SCPI_ResultDouble(context, ch->measure.i30);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Temperature(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	double data[8] = { 0 };

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->enabled) {
				data[i] = ch->measure.remote_temp;
			} else {
				data[i] = 0;
			}
		}
		SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		SCPI_ResultDouble(context, ch->measure.remote_temp);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_OutputPower(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	double data[8] = { 0 };

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->enabled) {
				data[i] = ch->measure.fwd_pwr;
			} else {
				data[i] = 0;
			}
		}
		SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		SCPI_ResultDouble(context, ch->measure.fwd_pwr);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_ReversePower(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	double data[8] = { 0 };

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->enabled) {
				data[i] = ch->measure.rfl_pwr;
			} else {
				data[i] = 0;
			}
		}
		SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		SCPI_ResultDouble(context, ch->measure.rfl_pwr);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t Interlock_Clear(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		rf_clear_interlock();
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		ch->input_interlock = false;
		ch->output_interlock = false;
		ch->soft_interlock = false;

		vTaskDelay(10);

		uint8_t chan = rf_channels_read_enabled() & (1 << channel);
		uint8_t channel_mask = rf_channels_get_mask() & chan;

		rf_channels_sigon(channel_mask, true);
		led_bar_or(rf_channels_read_sigon(), 0, 0);
		led_bar_and(0x00, (1 << channel), 0x00);
		return SCPI_RES_OK;
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t Interlock_StatusQ(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	uint8_t mask = 0;

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->input_interlock || ch->output_interlock) {
				mask |= 1UL << i;
			}
		}
		SCPI_ResultUInt8(context, mask);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		if (ch->input_interlock || ch->output_interlock) {
			SCPI_ResultBool(context, true);
			return SCPI_RES_OK;
		} else {
			SCPI_ResultBool(context, false);
			return SCPI_RES_OK;
		}
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t Interlock_OverloadQ(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	uint8_t mask = 0;

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->input_interlock || ch->output_interlock) {
				mask |= 1UL << i;
			}
		}
		SCPI_ResultUInt8(context, mask);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		if (ch->input_interlock || ch->output_interlock) {
			SCPI_ResultBool(context, true);
			return SCPI_RES_OK;
		} else {
			SCPI_ResultBool(context, false);
			return SCPI_RES_OK;
		}
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

static scpi_result_t Interlock_ErrorQ(scpi_t * context)
{
	uint32_t channel;
	uint8_t ch_mask = rf_channels_get_mask();
	channel_t * ch;
	uint8_t mask = 0;

	if (!SCPI_ParamUInt32(context, &channel, false)) {
		channel = ch_mask;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel == ch_mask) {
		for (int i = 0; i < 8; i++) {
			ch = rf_channel_get(i);
			if (ch->error) {
				mask |= 1UL << i;
			}
		}
		SCPI_ResultUInt8(context, mask);
		return SCPI_RES_OK;
	}

	if (channel < 8) {
		ch = rf_channel_get(channel);
		if (ch->error) {
			SCPI_ResultBool(context, true);
			return SCPI_RES_OK;
		} else {
			SCPI_ResultBool(context, false);
			return SCPI_RES_OK;
		}
	} else {
		return SCPI_RES_ERR;
	}

	return SCPI_RES_OK;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
    { .pattern = "*OPC", .callback = SCPI_CoreOpc,},
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
    { .pattern = "*RST", .callback = SCPI_CoreRst,},
    { .pattern = "*SRE", .callback = SCPI_CoreSre,},
    { .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
    { .pattern = "*TST?", .callback = My_CoreTstQ,},
    { .pattern = "*WAI", .callback = SCPI_CoreWai,},

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {.pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
    {.pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
    {.pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

    /* {.pattern = "STATus:OPERation?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:EVENt?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:CONDition?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle?", .callback = scpi_stub_callback,}, */

    {.pattern = "STATus:QUEStionable[:EVENt]?", .callback = SCPI_StatusQuestionableEventQ,},
    /* {.pattern = "STATus:QUEStionable:CONDition?", .callback = scpi_stub_callback,}, */
    {.pattern = "STATus:QUEStionable:ENABle", .callback = SCPI_StatusQuestionableEnable,},
    {.pattern = "STATus:QUEStionable:ENABle?", .callback = SCPI_StatusQuestionableEnableQ,},
    {.pattern = "STATus:PRESet", .callback = SCPI_StatusPreset,},

	/* Channel control */
	{.pattern = "CHANnel:ENABle", .callback = CHANNEL_Enable,},
	{.pattern = "CHANnel:DISABle", .callback = CHANNEL_Disable,},
	{.pattern = "CHANnel:ENABle?", .callback = CHANNEL_EnableQ,},

	/* Data gathering */
	{.pattern = "MEASure:CURRent?", .callback = CHANNEL_Current,},
	{.pattern = "MEASure:TEMPerature?", .callback = CHANNEL_Temperature,},
	{.pattern = "MEASure:OUTput?", .callback = CHANNEL_OutputPower,},
	{.pattern = "MEASure:REVerse?", .callback = CHANNEL_ReversePower,},
	{.pattern = "MEASure:FAN?", .callback = MEASURE_FanQ,},

	/* Interlocks */
	{.pattern = "INTerlock:POWer", .callback = INTERLOCK_Power,},
	{.pattern = "INTerlock:CLEar", .callback = Interlock_Clear,},
	{.pattern = "INTerlock:STATus?", .callback = Interlock_StatusQ,},
	{.pattern = "INTerlock:OVERload?", .callback = Interlock_OverloadQ,},
	{.pattern = "INTerlock:ERRor?", .callback = Interlock_ErrorQ,},
	{.pattern = "INTerlock:POWer?", .callback = INTERLOCK_PowerQ,},

    SCPI_CMD_LIST_END
};

scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;
