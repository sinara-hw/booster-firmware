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
#include "scpi/scpi.h"
#include "scpi-def.h"

#include "adc.h"
#include "channels.h"

extern channel_t channels[8];

/**
 * Reimplement IEEE488.2 *TST?
 *
 * Result should be 0 if everything is ok
 * Result should be 1 if something goes wrong
 *
 * Return SCPI_RES_OK
 */
static scpi_result_t My_CoreTstQ(scpi_t * context) {

    SCPI_ResultInt32(context, 0);

    return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Control(scpi_t * context, bool enable)
{
	int32_t param_array[8] = { 0 };
	size_t param_count = 0;
	uint32_t mask;

	fprintf(stderr, enable ? "CHANnel:ENABle\r\n" : "CHANnel:DISABle\r\n"); /* debug command name */

//	/* read first parameter if present */
//	if (!SCPI_ParamArrayInt32(context, param_array, 8, &param_count, SCPI_FORMAT_ASCII, true)) {
//		return SCPI_RES_ERR;
//	}

	if (!SCPI_ParamUInt32(context, &mask, true)) {
		return SCPI_RES_ERR;
	}

	uint8_t channel_mask = 0;
	rf_channels_control(channel_mask, enable);

	SCPI_ResultBool(context, enable);
	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Enable(scpi_t * context)
{
	return CHANNEL_Control(context, true);
}

static scpi_result_t CHANNEL_Disable(scpi_t * context)
{
	return CHANNEL_Control(context, false);
}

static scpi_result_t CHANNEL_Toggle(scpi_t * context)
{
	static bool toggle = 0;
	return CHANNEL_Control(context, toggle);
	toggle = !toggle;
}

static scpi_result_t CHANNEL_List(scpi_t * context)
{
	uint8_t channels[8] = { 0 };
	uint8_t count = rf_available_channels(channels);

	SCPI_ResultArrayUInt8(context, channels, (size_t) count, SCPI_FORMAT_ASCII);
	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_Status(scpi_t * context)
{
	uint8_t status[8] = { 0 };

	for (int i = 0; i < 8; i++)
	{
		if (channels[i].enabled) status[i] |= 1;
		if (channels[i].sigon) status[i] |= 2;
		if (channels[i].alert) status[i] |= 4;
		if (channels[i].overvoltage) status[i] |= 8;
	}

	SCPI_ResultArrayUInt8(context, status, (size_t) 8, SCPI_FORMAT_ASCII);
	return SCPI_RES_OK;
}

static scpi_result_t MEASURE_Voltage(scpi_t * context)
{
	/* Returned as float array, 2 measurements per channel */
	float voltage[16] = { 0 };

	int j = 0;
	for (int i = 0, j = 0; i < 8; i++, j += 2)
	{
		voltage[j] = channels[i].adc_ch1;
		voltage[j + 1] = channels[i].adc_ch2;
	}

	SCPI_ResultArrayFloat(context, voltage, (size_t) 16, SCPI_FORMAT_ASCII);
	return SCPI_RES_OK;
}

static scpi_result_t MEASURE_Current(scpi_t * context)
{
	/* Returned as float array, 2 measurements per channel */
	uint16_t current[24] = { 0 };

	for (int i = 0, j = 0; i < 8; i++, j += 3)
	{
		current[j] = channels[i].pwr_ch1;
		current[j + 1] = channels[i].pwr_ch2;
		current[j + 2] = channels[i].pwr_ch3;
	}

	SCPI_ResultArrayUInt16(context, current, (size_t) 24, SCPI_FORMAT_ASCII);
	return SCPI_RES_OK;
}

static scpi_result_t TEST_Array(scpi_t * context) {
    int32_t param_array[8] = { 0 };
    size_t param_count = 0;

    fprintf(stderr, "TEST:Array\r\n"); /* debug command name */

    /* read first parameter if present */
    if (!SCPI_ParamArrayInt32(context, param_array, 8, &param_count, SCPI_FORMAT_ASCII, true)) {
        return SCPI_RES_ERR;
    }

    for (int i = 0; i < param_count; i++)
    {
    	printf("param[%d] = %d\n", i, (int) param_array[i]);
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

	/* CHANNEL CONTROL */
	{.pattern = "CHANnel:ENABle", .callback = CHANNEL_Enable,},
	{.pattern = "CHANnel:DISABle", .callback = CHANNEL_Disable,},
	{.pattern = "CHANnel:STATus?", .callback = CHANNEL_Status,},
	{.pattern = "CHANnel:LIST?", .callback = CHANNEL_List,},
	{.pattern = "CHANnel:MEASure:VOLTage?", .callback = MEASURE_Voltage,},
	{.pattern = "CHANnel:MEASure:CURRent?", .callback = MEASURE_Current,},

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
