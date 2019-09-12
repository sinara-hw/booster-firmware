/*-
no
beka
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
#include "device.h"

scpi_bool_t SCPI_ParamIsDouble(scpi_parameter_t * parameter)
{
	// check if parameter has either dot or e - eg. 1.1 or 1e1
	if (strchr(parameter->ptr, '.') != NULL || strchr(parameter->ptr, 'e') != NULL || strchr(parameter->ptr, 'E') != NULL)
		return TRUE;

	return FALSE;
}

static scpi_result_t SCPI_ReturnString(scpi_t * context, char * buffer)
{
	int len = strlen(buffer);
	SCPI_ResultCharacters(context, buffer, len);
	return SCPI_RES_OK;
}

static scpi_result_t IDN_Query(scpi_t * context)
{
	char buffer[128] = { 0x00 };
	uint32_t b2 = (*(uint32_t *) (0x1FFF7A10 + 4));

	device_t * dev = device_get_config();
	int len = sprintf(buffer, "RFPA %s, built %s %s, id %04X, hw rev 1.%d", VERSION_STRING, __DATE__, __TIME__, b2, dev->hw_rev);

	SCPI_ResultCharacters(context, buffer, len);
	return SCPI_RES_OK;
}

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
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	uint8_t ch_mask = rf_channels_get_mask();

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & ch_mask) {
					rf_channels_enable(1 << intval);
					return SCPI_ReturnString(context, "OK");
				} else
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				rf_channels_enable(ch_mask);
				return SCPI_ReturnString(context, "OK");
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t CHANNEL_Disable(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	uint8_t ch_mask = rf_channels_get_mask();

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & ch_mask) {
					rf_channels_disable(1 << intval);
					return SCPI_ReturnString(context, "OK");
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				rf_channels_disable(ch_mask);
				return SCPI_ReturnString(context, "OK");
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t INTERLOCK_Power(scpi_t * context)
{
	uint32_t channel;
	double interlock;
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;

	scpi_choice_def_t bool_options[] = {
		{"MAX", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	/* Get channel number */
	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (!SCPI_ParamToUInt32(context, &param, &channel)) {
		return SCPI_RES_ERR;
	}

	/* Get interlock value */
	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel < 8) {
		if (result) {
			if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
				if (!SCPI_ParamToDouble(context, &param, &interlock)) {
					return SCPI_RES_ERR;
				}

				if (interlock >= 0 && interlock <= 38.0) {
					rf_channel_interlock_set(channel, interlock);
					return SCPI_ReturnString(context, "OK");
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -98, \"Interlock value invalid (0-38 dBm)\"");
				}
			} else {
				result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
				if (intval) {
					ch = rf_channel_get((uint8_t) channel);
					ch->cal_values.output_dac_cal_value = (uint16_t) 4095;

					if (ch->enabled) {
						if (lock_take(I2C_LOCK, portMAX_DELAY))
						{
							i2c_mux_select((uint8_t) channel);
							i2c_dual_dac_set(1, ch->cal_values.output_dac_cal_value);
							lock_free(I2C_LOCK);
						}
					}

					return SCPI_ReturnString(context, "OK");
				}
			}
		}
	} else {
		return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
	}

	return SCPI_RES_ERR;
}

static scpi_result_t INTERLOCK_PowerQ(scpi_t * context)
{
	uint32_t channel;
	scpi_bool_t result;
	scpi_parameter_t param;

	/* Get channel number */
	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (!SCPI_ParamToUInt32(context, &param, &channel)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (channel < 8) {
		double value = rf_channel_interlock_get(channel);
		SCPI_ResultDouble(context, value);

		return SCPI_RES_OK;
	} else {
		return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_EnableQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					SCPI_ResultBool(context, ch->enabled);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->enabled) {
						mask |= 1UL << i;
					}
				}
				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_OK;
}

static scpi_result_t CHANNEL_DetectQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					SCPI_ResultBool(context, 1);
					return SCPI_RES_OK;
				} else {
					SCPI_ResultBool(context, 0);
					return SCPI_RES_OK;
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->detected) {
						mask |= 1UL << i;
					}
				}
				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
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
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	double data[8] = { 0 };

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					SCPI_ResultDouble(context, ch->measure.i30);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
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
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t CHANNEL_Temperature(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	double data[8] = { 0 };

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					SCPI_ResultDouble(context, ch->measure.remote_temp);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->detected) {
						data[i] = ch->measure.remote_temp;
					} else {
						data[i] = 0;
					}
				}

				SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t CHANNEL_OutputPower(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	double data[8] = { 0 };

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					double tmp = ch->measure.fwd_pwr;
					SCPI_ResultDouble(context, tmp);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
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
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t CHANNEL_InputPower(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	double data[8] = { 0 };

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					double tmp = ch->measure.input_power;
					SCPI_ResultDouble(context, tmp);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->enabled) {
						data[i] = ch->measure.input_power;
					} else {
						data[i] = 0;
					}
				}

				SCPI_ResultArrayDouble(context, data, 8, SCPI_FORMAT_ASCII);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t CHANNEL_ReversePower(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	double data[8] = { 0 };

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					SCPI_ResultDouble(context, ch->measure.rfl_pwr);
					return SCPI_RES_OK;
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
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
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t Interlock_Clear(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t ch_mask = rf_channels_get_mask();

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & ch_mask) {
					rf_channel_clear_interlock(intval);
					return SCPI_ReturnString(context, "OK");
				} else
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				rf_clear_interlock();
				return SCPI_ReturnString(context, "OK");
			}
		}
	}

	return SCPI_RES_OK;
}

static scpi_result_t Interlock_DiagQ(scpi_t * context)
{
	uint32_t intval = 0;
	channel_t * ch;
	scpi_bool_t result;
	scpi_parameter_t param;

	/* Get channel number */
	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (!SCPI_ParamToUInt32(context, &param, &intval)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	double results[21] = { 0x00 };
	if (intval >= 0 && intval < 8)
	{
		ch = rf_channel_get(intval);

		results[0] = ch->detected;
		results[1] = ch->enabled;
		results[2] = ch->output_interlock;
		results[3] = ch->input_interlock;
		results[4] = ch->measure.adc_pwr_ch1;
		results[5] = ch->measure.adc_pwr_ch2;
		results[6] = ch->measure.i30;
		results[7] = ch->measure.i60;
		results[8] = ch->measure.p5v0mp;
		results[9] = ch->measure.remote_temp;
		results[10] = ch->measure.fwd_pwr;
		results[11] = ch->measure.rfl_pwr;
		results[12] = ch->measure.input_power;
		results[13] = temp_mgt_get_fanspeed();
		results[14] = ch->error;
		results[15] = ch->hwid[0];
		results[16] = ch->hwid[1];
		results[17] = ch->hwid[2];
		results[18] = ch->hwid[3];
		results[19] = ch->hwid[4];
		results[20] = ch->hwid[5];

		SCPI_ResultArrayDouble(context, results, 21, SCPI_FORMAT_ASCII);
		return SCPI_RES_OK;
	} else {
		return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
	}

	return SCPI_RES_ERR;
}

static scpi_result_t Interlock_StatusQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8)
			{
				ch = rf_channel_get(intval);
				if (ch->input_interlock || ch->output_interlock || ch->soft_interlock || ch->error) {
					SCPI_ResultBool(context, true);
					return SCPI_RES_OK;
				} else {
					SCPI_ResultBool(context, false);
					return SCPI_RES_OK;
				}
			}
			return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->input_interlock || ch->output_interlock || ch->soft_interlock || ch->error) {
						mask |= 1UL << i;
					}
				}
				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t Interlock_FORwardQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					if (ch->output_interlock) {
						SCPI_ResultBool(context, true);
						return SCPI_RES_OK;
					} else {
						SCPI_ResultBool(context, false);
						return SCPI_RES_OK;
					}
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");;
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->output_interlock) {
						mask |= 1UL << i;
					}
				}
				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t Interlock_REVerseQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					if (ch->soft_interlock) {
						SCPI_ResultBool(context, true);
						return SCPI_RES_OK;
					} else {
						SCPI_ResultBool(context, false);
						return SCPI_RES_OK;
					}
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");;
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->soft_interlock) {
						mask |= 1UL << i;
					}
				}
				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

static scpi_result_t Interlock_ErrorQ(scpi_t * context)
{
	scpi_bool_t result;
	scpi_parameter_t param;
	int32_t intval = 0;
	channel_t * ch;
	uint8_t mask = 0;

	scpi_choice_def_t bool_options[] = {
		{"ALL", 1},
		SCPI_CHOICE_LIST_END /* termination of option list */
	};

	result = SCPI_Parameter(context, &param, true);
	if (!result) {
		return SCPI_RES_ERR;
	}

	// throw error if excess parameter is present
	if (SCPI_IsParameterPresent(context)) {
		return SCPI_RES_ERR;
	}

	if (SCPI_ParamIsDouble(&param)) {
		SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
		return SCPI_RES_ERR;
	}

	if (result) {
		if (param.type == SCPI_TOKEN_DECIMAL_NUMERIC_PROGRAM_DATA) {
			if (!SCPI_ParamToInt32(context, &param, &intval)) {
				return SCPI_RES_ERR;
			}

			if (intval >= 0 && intval < 8) {
				if ((1 << intval) & rf_channels_get_mask()) {
					ch = rf_channel_get(intval);
					if (ch->error) {
						SCPI_ResultBool(context, true);
						return SCPI_RES_OK;
					} else {
						SCPI_ResultBool(context, false);
						return SCPI_RES_OK;
					}
				} else {
					return SCPI_ReturnString(context, "[scpi] **ERROR: -99, \"Channel not detected\"");;
				}
			} else {
				return SCPI_ReturnString(context, "[scpi] **ERROR: -97, \"Wrong channel selected (0-7)\"");
			}
		} else {
			result = SCPI_ParamToChoice(context, &param, bool_options, &intval);
			if (intval) {
				for (int i = 0; i < 8; i++) {
					ch = rf_channel_get(i);
					if (ch->error) {
						mask |= 1UL << i;
					}
				}

				SCPI_ResultUInt8(context, mask);
				return SCPI_RES_OK;
			}
		}
	}

	return SCPI_RES_ERR;
}

const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
//    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
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

	{ .pattern = "*IDN?", .callback = IDN_Query,},
	/* Channel control */
	{.pattern = "CHANnel:ENABle", .callback = CHANNEL_Enable,},
	{.pattern = "CHANnel:DISABle", .callback = CHANNEL_Disable,},
	{.pattern = "CHANnel:ENABle?", .callback = CHANNEL_EnableQ,},
	{.pattern = "CHANnel:DETect?", .callback = CHANNEL_DetectQ,},
	{.pattern = "CHANnel:DIAGnostics?", .callback = Interlock_DiagQ,},

	/* Data gathering */
	{.pattern = "MEASure:CURRent?", .callback = CHANNEL_Current,},
	{.pattern = "MEASure:TEMPerature?", .callback = CHANNEL_Temperature,},
	{.pattern = "MEASure:OUTput?", .callback = CHANNEL_OutputPower,},
	{.pattern = "MEASure:INput?", .callback = CHANNEL_InputPower,},
	{.pattern = "MEASure:REVerse?", .callback = CHANNEL_ReversePower,},
	{.pattern = "MEASure:FAN?", .callback = MEASURE_FanQ,},

	/* Interlocks */
	{.pattern = "INTerlock:POWer", .callback = INTERLOCK_Power,},
	{.pattern = "INTerlock:CLEar", .callback = Interlock_Clear,},
	{.pattern = "INTerlock:STATus?", .callback = Interlock_StatusQ,},
	{.pattern = "INTerlock:FORward?", .callback = Interlock_FORwardQ,},
	{.pattern = "INTerlock:REVerse?", .callback = Interlock_REVerseQ,},
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
