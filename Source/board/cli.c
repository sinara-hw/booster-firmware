/*
 * cli.c
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */
#include "config.h"
#include "cli.h"
#include "network.h"
#include "usb.h"
#include "i2c.h"
#include "channels.h"
#include "locks.h"
#include "eeprom.h"
#include "led_bar.h"
#include "tasks.h"
#include "ads7924.h"

#include "FreeRTOS_CLI.h"

#define MAX_INPUT_LENGTH    64
#define MAX_OUTPUT_LENGTH   128

extern xQueueHandle _xRxQueue;

static const int8_t * const pcWelcomeMessage = (int8_t *) "[cli] Type Help to view a list of registered commands";

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xTaskStats =
{
	"task-stats", /* The command string to type. */
	"task-stats:\r\n Displays a table showing the state of each FreeRTOS task\r\n",
	prvTaskStatsCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xChCtrl =
{
	"ctrl", /* The command string to type. */
	"ctrl:\r\n Enables or disables selected channels\r\n",
	prvCtrlCommand, /* The function to run. */
	1 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xRebootDevice =
{
	"reboot", /* The command string to type. */
	"reboot:\r\n Reboots the device\r\n",
	prvRebootCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xBootlog =
{
	"bootlog", /* The command string to type. */
	"bootlog:\r\n Shows logs from device boot\r\n",
	prvBootlogCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xIPStats =
{
	"ifconfig", /* The command string to type. */
	"ifconfig:\r\n Displays network configuration info\r\n",
	prvNetworkConfigCommand, /* The function to run. */
	-1 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xMACChange =
{
	"macaddress", /* The command string to type. */
	"macaddress:\r\n Change mac address\r\n",
	prvNetworkMACChange, /* The function to run. */
	1 /* Dynamic number of parameters. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xDACControl =
{
	"dac", /* The command string to type. */
	"dac:\r\n Use i2c interface\r\n",
	prvDacCommand, /* The function to run. */
	3 /* Dynamic number of parameters. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xEEPromW =
{
	"eepromw", /* The command string to type. */
	"eepromw:\r\n Write eeprom memory\r\n",
	prvEEPROMWCommand, /* The function to run. */
	3 /* Dynamic number of parameters. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xEEPromR =
{
	"eepromr", /* The command string to type. */
	"eepromr:\r\n Read eeprom memory\r\n",
	prvEEPROMRCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xStatusControl =
{
	"chstatus", /* The command string to type. */
	"chstatus:\r\n Check channel status\r\n",
	prvCHSTATUSCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xI2CControl =
{
	"i2c", /* The command string to type. */
	"i2c:\r\n Use i2c interface\r\n",
	prvI2CCommand, /* The function to run. */
	-1 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xStartCControl =
{
	"start", /* The command string to type. */
	"start:\r\n Start channel task\r\n",
	prvStartCommand, /* The function to run. */
	0 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xStopCControl =
{
	"stop", /* The command string to type. */
	"stop:\r\n Stop channel task\r\n",
	prvStopCommand, /* The function to run. */
	0 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xRestartCControl =
{
	"reset", /* The command string to type. */
	"reset:\r\n Reset channels interlock\r\n",
	prvResetIntCommand, /* The function to run. */
	0 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xCalibrateChannel =
{
	"cal", /* The command string to type. */
	"cal:\r\n Calibrate channel n\r\n",
	prvCalibrateChannelCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xBIASChannel =
{
	"bias", /* The command string to type. */
	"bias:\r\n Set BIAS voltage\r\n",
	prvBIASCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xADCReadout =
{
	"adc", /* The command string to type. */
	"adc:\r\n Read raw adc values\r\n",
	prvADCCommand, /* The function to run. */
	1 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xCALCPWRCommand =
{
	"calc", /* The command string to type. */
	"calc:\r\n Cal PWR Readouts\r\n",
	prvCALPWRCommand, /* The function to run. */
	3 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xCALCCLRCommand =
{
	"ccal", /* The command string to type. */
	"ccal:\r\n Cal PWR Readouts\r\n",
	prvClearCalibrationCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xCalibrateChannelManual =
{
	"mcal", /* The command string to type. */
	"mcal:\r\n Calibrate channel manually n\r\n",
	prvCalibrateChannelManualCommand, /* The function to run. */
	3 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xCalibrateBias =
{
	"calbias", /* The command string to type. */
	"calbias:\r\n Calibrate channel manually n\r\n",
	prvCalibrateBIASCommand, /* The function to run. */
	2 /* Dynamic number of parameters. */
};

static const CLI_Command_Definition_t xClearAlertCMD =
{
	"clearal", /* The command string to type. */
	"clearal:\r\n clear current alert n\r\n",
	prvClearAlert, /* The function to run. */
	1 /* Dynamic number of parameters. */
};

BaseType_t prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *const pcHeader = "Task\t\tState\tPriority\tStack\t#\r\n**************************************************\r\n";

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, pcHeader );
	vTaskList( pcWriteBuffer + strlen( pcHeader ) );

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

BaseType_t prvRebootCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	NVIC_SystemReset();

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

BaseType_t prvClearAlert( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring clear alert command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (lock_take(I2C_LOCK, portMAX_DELAY))
				{
					i2c_mux_select(channel);
					ads7924_clear_alert();
					lock_free(I2C_LOCK);
				}
			}


            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	vTaskSuspend(task_rf_interlock);
	vTaskSuspend(task_rf_info);

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

BaseType_t prvStartCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	vTaskResume(task_rf_interlock);
	vTaskResume(task_rf_info);

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

BaseType_t prvResetIntCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

//	rf_clear_interlock();

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

static uint16_t cal_in_val1 = 0;
static uint16_t cal_in_pwr1 = 0;
static uint16_t cal_in_val2 = 0;
static uint16_t cal_in_pwr2 = 0;

static uint16_t cal_rfl_val1 = 0;
static uint16_t cal_rfl_pwr1 = 0;
static uint16_t cal_rfl_val2 = 0;
static uint16_t cal_rfl_pwr2 = 0;

BaseType_t prvCALPWRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t pwr_type;
	static uint8_t pwr_cal;
	channel_t * ch;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		channel_t * ch;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) pwr_type = atoi((char*) pcParameter);
			if (lParameterNumber == 3) pwr_cal = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring cal command, ch %d pwrt %d pwr %d\n\r", channel, pwr_type, pwr_cal);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {

				ch = rf_channel_get(channel);

				if (pwr_type == 1) {
					printf("Value1 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch1);
					cal_in_pwr1 = pwr_cal;
					cal_in_val1 = ch->measure.adc_raw_ch1;
				} else if (pwr_type == 2) {
					printf("Value2 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch1);
					cal_in_pwr2 = pwr_cal;
					cal_in_val2 = ch->measure.adc_raw_ch1;
				} else if (pwr_type == 3) {
					printf("Value1 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch2);
					cal_rfl_pwr1 = pwr_cal;
					cal_rfl_val1 = ch->measure.adc_raw_ch2;
				} else if (pwr_type == 4) {
					printf("Value2 for pwr %d = %d\n", pwr_cal, ch->measure.adc_raw_ch2);
					cal_rfl_pwr2 = pwr_cal;
					cal_rfl_val2 = ch->measure.adc_raw_ch2;
				}

				if (cal_in_val1 != 0 && cal_in_val2 != 0)
				{
					printf("Calculating points for %d -> %d\n", cal_in_pwr1, cal_in_pwr2);
					uint16_t a = ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2));
					uint16_t b = (cal_in_val1 - ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2)) * cal_in_pwr1);
					printf("INPWR A = %d B = %d\n", a, b);

					ch = rf_channel_get(channel);
					// save values to eeprom
					ch->cal_values.fwd_pwr_offset = b;

					if (lock_take(I2C_LOCK, portMAX_DELAY)) {
						i2c_mux_select(channel);

						eeprom_write16(ADC1_OFFSET_ADDRESS, b);
						vTaskDelay(100);
						eeprom_write16(ADC1_SCALE_ADDRESS, a);
						lock_free(I2C_LOCK);
					}

					ch->cal_values.fwd_pwr_scale = a;
					cal_in_val1 = 0;
					cal_in_pwr1 = 0;
					cal_in_val2 = 0;
					cal_in_pwr2 = 0;
				}

				if (cal_rfl_val1 != 0 && cal_rfl_val2 != 0)
				{
					printf("Calculating points for %d -> %d\n", cal_rfl_pwr1, cal_rfl_pwr2);
					uint16_t a = ((cal_rfl_val1 - cal_rfl_val2) / (cal_rfl_pwr1 - cal_rfl_pwr2));
					uint16_t b = (cal_rfl_val1 - ((cal_rfl_val1 - cal_rfl_val2) / (cal_rfl_pwr1 - cal_rfl_pwr2)) * cal_rfl_pwr1);
					printf("RFLPWR A = %d B = %d\n", a, b);

					ch = rf_channel_get(channel);
					// save values to eeprom
					ch->cal_values.rfl_pwr_offset = a;
					ch->cal_values.rfl_pwr_scale = b;

					if (lock_take(I2C_LOCK, portMAX_DELAY)) {
						i2c_mux_select(channel);

						eeprom_write16(ADC2_OFFSET_ADDRESS, b);
						vTaskDelay(100);
						eeprom_write16(ADC2_SCALE_ADDRESS, a);
						lock_free(I2C_LOCK);
					}

					cal_rfl_val1 = 0;
					cal_rfl_pwr1 = 0;
					cal_rfl_val2 = 0;
					cal_rfl_pwr2 = 0;
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}


BaseType_t prvADCCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	channel_t * ch;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring adc command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				ch = rf_channel_get(channel);
				printf("ADCs %d %d\n", ch->measure.adc_raw_ch1, ch->measure.adc_raw_ch2);
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvDacCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t dac_ch;
	static uint16_t value;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		dac_ch = 0;
		value = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) dac_ch = atoi((char*) pcParameter);
			if (lParameterNumber == 3) value = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring dac command, ch %d dac %d value %d\n\r", channel, dac_ch, value);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (dac_ch < 3) {
					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(channel);
						i2c_dual_dac_set(dac_ch, value);

						lock_free(I2C_LOCK);
					}
				}
			}


            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvCalibrateChannelCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t type;
	channel_t * ch;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) type = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring cal command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */
			uint16_t dacval = 1500;
			uint16_t retval = 0;

			if (channel < 8)
			{
				if (type == 0) {
					printf("Calibrating input interlock\n");
					retval = rf_channel_calibrate_input_interlock(channel, dacval, 100);
					if (retval == 0) retval = 100;
					printf("Calibration step = 100 completed = %d\n", retval);

					vTaskDelay(500);

					retval *= 1.3;
					retval = rf_channel_calibrate_input_interlock(channel, retval, 10);
					if (retval == 0) retval = 10;

					vTaskDelay(500);

					printf("Calibration step = 10 completed = %d\n", retval);
					retval *= 2;
					retval = rf_channel_calibrate_input_interlock(channel, retval, 1);
					if (retval != 0) {
						printf("Calibration step = 1 completed = %d\n", retval);

						ch = rf_channel_get(channel);
						if (lock_take(I2C_LOCK, portMAX_DELAY)) {
							i2c_mux_select(channel);

							ch->cal_values.input_dac_cal_value = retval;
							eeprom_write16(DAC1_EEPROM_ADDRESS, retval);
							lock_free(I2C_LOCK);
						}
					} else {
						printf("Calibration failed\n");
					}
				}

				if (type == 1) {
					printf("Calibrating output interlock\n");
					retval = rf_channel_calibrate_output_interlock(channel, dacval, 100);
					if (retval == 0) retval = 100;
					printf("Calibration step = 100 completed = %d\n", retval);

					vTaskDelay(500);

					retval *= 1.3;
					retval = rf_channel_calibrate_output_interlock(channel, retval, 10);
					if (retval == 0) retval = 10;

					vTaskDelay(500);

					printf("Calibration step = 10 completed = %d\n", retval);
					retval *= 1.1;
					retval = rf_channel_calibrate_output_interlock(channel, retval, 1);
					if (retval != 0) {
						printf("Calibration step = 1 completed = %d\n", retval);

						ch = rf_channel_get(channel);
						if (lock_take(I2C_LOCK, portMAX_DELAY)) {
							i2c_mux_select(channel);

							ch->cal_values.output_dac_cal_value = retval;
							eeprom_write16(DAC2_EEPROM_ADDRESS, retval);
							lock_free(I2C_LOCK);
						}

					} else {
						printf("Calibration failed\n");
					}
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvCalibrateBIASCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t val;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) val = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring cal command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8)
			{
				rf_channel_calibrate_bias(channel, val);
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}


BaseType_t prvCalibrateChannelManualCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t type;
	static uint16_t start;
	channel_t * ch;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) type = atoi((char*) pcParameter);
			if (lParameterNumber == 3) start = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring cal command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */
			uint16_t retval = 0;

			if (channel < 8)
			{
				if (type == 0) {
					retval = rf_channel_calibrate_input_interlock(channel, start, 1);
					if (retval != 0) {
						printf("Calibration step = 1 completed = %d\n", retval);

						ch = rf_channel_get(channel);
						if (lock_take(I2C_LOCK, portMAX_DELAY)) {
							i2c_mux_select(channel);

							ch->cal_values.input_dac_cal_value = retval;
							eeprom_write16(DAC1_EEPROM_ADDRESS, retval);
							lock_free(I2C_LOCK);
						}

					} else {
						printf("Calibration failed\n");
					}
				}

				if (type == 1) {
					retval = rf_channel_calibrate_output_interlock(channel, start, 1);
					if (retval != 0) {
						printf("Calibration step = 1 completed = %d\n", retval);

						ch = rf_channel_get(channel);
						if (lock_take(I2C_LOCK, portMAX_DELAY)) {
							i2c_mux_select(channel);

							ch->cal_values.output_dac_cal_value = retval;
							eeprom_write16(DAC2_EEPROM_ADDRESS, retval);
							lock_free(I2C_LOCK);
						}

					} else {
						printf("Calibration failed\n");
					}
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvClearCalibrationCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint16_t value;
	channel_t * ch;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) value = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring clear cal command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8)
			{
				ch = rf_channel_get(channel);

				ch->cal_values.input_dac_cal_value = value;
				ch->cal_values.output_dac_cal_value = value;

				if (lock_take(I2C_LOCK, portMAX_DELAY)) {
					i2c_mux_select(channel);

					eeprom_write16(DAC1_EEPROM_ADDRESS, value);
					vTaskDelay(50);
					eeprom_write16(DAC2_EEPROM_ADDRESS, value);
					lock_free(I2C_LOCK);
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvBIASCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint16_t value;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		value = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) value = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring bias command, ch %d value %d\n\r", channel, value);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (lock_take(I2C_LOCK, portMAX_DELAY))
				{
					i2c_mux_select(channel);
					i2c_dac_set(value);
					lock_free(I2C_LOCK);
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvEEPROMWCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t address;
	static uint16_t value;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		address = 0;
		value = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) address = atoi((char*) pcParameter);
			if (lParameterNumber == 3) value = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring eepromw command, ch %d dac %d value %d\n\r", channel, address, value);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (address < 128) {
					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(channel);
						eeprom_write(address, value);

						lock_free(I2C_LOCK);
					}
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvEEPROMRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t address;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		address = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) address = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring eepromR command, ch %d dac %d\n\r", channel, address);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (address < 128) {
					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(channel);
						printf("EEPROM Reg[%d] = %d\n", address, eeprom_read(address));

						lock_free(I2C_LOCK);
					}
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvCHSTATUSCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channel;
	static uint8_t address;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		channel = 0;
		address = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channel = atoi((char*) pcParameter);
			if (lParameterNumber == 2) address = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring status command, ch %d dac %d value %d\n\r", channel, address);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (channel < 8) {
				if (address == 0) {
					uint8_t val = rf_channels_read_ovl();
					printf("Status %d\n", (val >> channel) & 0x01);
				} else if (address == 1) {
					uint8_t val = rf_channels_read_user();
					printf("Status %d\n", (val >> channel) & 0x01);
				}
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvCtrlCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channels;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if ( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;
		channels = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) channels = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;
		} else {
			rf_channels_enable(channels);

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;
		}
    }

	return xReturn;
}

BaseType_t prvI2CCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
//	( void ) pcCommandString;
//	( void ) xWriteBufferLen;
//	configASSERT(pcWriteBuffer);

	static uint8_t i2c_addr, i2c_register, i2c_data;
	static int i2c_rw;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
	{
		/* Next time the function is called the first parameter will be echoed
		back. */
		lParameterNumber = 1L;

		i2c_addr = 0;
		i2c_register = 0;
		i2c_data = 0;
		i2c_rw = 0;

		/* There is more data to be returned as no parameters have been echoed
		back yet, so set xReturn to pdPASS so the function will be called again. */
		xReturn = pdPASS;
	} else {
    	/* lParameter is not 0, so holds the number of the parameter that should
			be returned.  Obtain the complete parameter string. */
		pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                   (
                                       /* The command string itself. */
									   pcCommandString,
									   /* Return the next parameter. */
									   lParameterNumber,
									   /* Store the parameter string length. */
									   &lParameterStringLength
									);
		if( pcParameter != NULL )
		{
			// avoid buffer overflow
			if (lParameterStringLength > 15) lParameterStringLength = 15;
			if (lParameterNumber == 1) i2c_rw = atoi((char*) pcParameter);
			if (lParameterNumber == 2) i2c_addr = atoi((char*) pcParameter);
			if (lParameterNumber == 3) i2c_register = atoi((char*) pcParameter);
			if (lParameterNumber == 4) i2c_data= atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring i2c command, cmd %d, addr %d, reg %d, data %d\n", i2c_rw, i2c_addr, i2c_register, i2c_data);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */

			if (i2c_rw == 1) {
				i2c_write(I2C1, i2c_addr, i2c_register, i2c_data);
			} else if (i2c_rw == 0) {
				uint8_t ret = i2c_read(I2C1, i2c_addr, i2c_register);
				printf("i2c[%X][%X] = %X\n", i2c_addr, i2c_register, ret);
				printf("i2c[%d][%d] = %d\n", i2c_addr, i2c_register, ret);
			} else if (i2c_rw == 2) {
				i2c_scan_devices(true);
			} else if (i2c_rw == 3) {
				i2c_mux_select(i2c_addr);
			} else if (i2c_rw == 4) {
				i2c_start(I2C1, i2c_addr, I2C_Direction_Transmitter, 1);
				i2c_write_byte(I2C1, i2c_register);
				i2c_write_byte(I2C1, i2c_data);
				i2c_stop(I2C1);
			} else if (i2c_rw == 5) {
				i2c_mux_select(1);
				i2c_start(I2C1, 0x0E, I2C_Direction_Transmitter, 1);
				i2c_write_byte(I2C1, i2c_addr);
				i2c_write_byte(I2C1, i2c_register);
				i2c_write_byte(I2C1, i2c_data);
				i2c_stop(I2C1);
			}

            xReturn = pdFALSE;
			/* Start over the next time this command is executed. */
			lParameterNumber = 0;

			sprintf(pcWriteBuffer, "\r");
		}
    }

	return xReturn;
}

BaseType_t prvBootlogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	usb_bootlog();

	/* There is no more data to return after this single string, so return
	pdFALSE. */
	return pdFALSE;
}

uint8_t prvCheckValidIPAddress(uint8_t * ipstr, uint8_t * result)
{
	uint8_t lParameterCount = 0;
	char * ch;
	uint8_t num;

	ch = strtok((char*)ipstr, ".");
	while (ch != NULL) {
		num = atoi(ch);
		if (num >= 0 && num <= 255 && lParameterCount < 4) {
			result[lParameterCount++] = num;
			printf("IP [%d] = %d\n", lParameterCount, num);
		}
		ch = strtok(NULL, ".");
	}

	return lParameterCount;
}

uint8_t prvCheckValidMACAddress(uint8_t * ipstr, uint8_t * result)
{
	uint8_t lParameterCount = 0;
	char * ch;
	uint8_t num;

	ch = strtok((char*)ipstr, ":");
	while (ch != NULL) {
		num = strtol(ch, NULL, 16);
		if (num >= 0 && num <= 255 && lParameterCount < 6) {
			result[lParameterCount++] = num;
			printf("MAC [%d] = %d\n", lParameterCount, num);
		}
		ch = strtok(NULL, ":");
	}

	return lParameterCount;
}

BaseType_t prvNetworkMACChange( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t macaddr[17];

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

    if( lParameterNumber == 0 )
    {
        /* Next time the function is called the first parameter will be echoed
        back. */
        lParameterNumber = 1L;

    	memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		memset(macaddr, 0x00, 17);

        /* There is more data to be returned as no parameters have been echoed
        back yet, so set xReturn to pdPASS so the function will be called again. */
        xReturn = pdPASS;
    }
    else
    {
        /* lParameter is not 0, so holds the number of the parameter that should
        be returned.  Obtain the complete parameter string. */
        pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                    (
                                        /* The command string itself. */
                                        pcCommandString,
                                        /* Return the next parameter. */
                                        lParameterNumber,
                                        /* Store the parameter string length. */
                                        &lParameterStringLength
                                    );

        if( pcParameter != NULL )
        {
        	// avoid buffer overflow
        	if (lParameterStringLength > 17) lParameterStringLength = 17;

        	if (lParameterNumber == 1) {
				memcpy(macaddr, pcParameter, lParameterStringLength);
        	}

            xReturn = pdTRUE;
            lParameterNumber++;
        }
        else
        {
        	if (lParameterNumber == 2) {
        		uint8_t check = 1;
        		uint8_t macdata[6];

        		printf("macaddr %s\n", macaddr);
        		if (prvCheckValidMACAddress(macaddr, macdata) != 6) check = 0;

        		if (check) {
        			sprintf( pcWriteBuffer, "Success\n" );
        			set_mac_address(macdata);
        		} else
        			sprintf( pcWriteBuffer, "Invalid parameters\n" );
			} else
				sprintf( pcWriteBuffer, "Wrong number of parameters\r\n" );


            /* There is no more data to return, so this time set xReturn to
            pdFALSE. */
            xReturn = pdFALSE;

            /* Start over the next time this command is executed. */
            lParameterNumber = 0;
        }
    }

    return xReturn;
}

BaseType_t prvNetworkConfigCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t ipaddr[15];
	static uint8_t ipdst[15];
	static uint8_t subnet[15];

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

    if( lParameterNumber == 0 )
    {
        /* Next time the function is called the first parameter will be echoed
        back. */
        lParameterNumber = 1L;

    	memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		memset(ipaddr, 0x00, 15);
		memset(ipdst, 0x00, 15);
		memset(subnet, 0x00, 15);

        /* There is more data to be returned as no parameters have been echoed
        back yet, so set xReturn to pdPASS so the function will be called again. */
        xReturn = pdPASS;
    }
    else
    {
        /* lParameter is not 0, so holds the number of the parameter that should
        be returned.  Obtain the complete parameter string. */
        pcParameter = ( int8_t * ) FreeRTOS_CLIGetParameter
                                    (
                                        /* The command string itself. */
                                        pcCommandString,
                                        /* Return the next parameter. */
                                        lParameterNumber,
                                        /* Store the parameter string length. */
                                        &lParameterStringLength
                                    );

        if( pcParameter != NULL )
        {
        	// avoid buffer overflow
        	if (lParameterStringLength > 15) lParameterStringLength = 15;

        	if (lParameterNumber == 1) {
				memcpy(ipaddr, pcParameter, lParameterStringLength);
        	}

        	if (lParameterNumber == 2) {
				memcpy(ipdst, pcParameter, lParameterStringLength);
			}

        	if (lParameterNumber == 3) {
        		printf("param %s\n", pcParameter);
				memcpy(subnet, pcParameter, lParameterStringLength);
			}

            xReturn = pdTRUE;
            lParameterNumber++;
        }
        else
        {
        	if (lParameterNumber == 1) {
        		display_net_conf();
        	} else if (lParameterNumber == 2) {
        		if (!strcmp((char*) ipaddr, "dhcp")) prvDHCPTaskRestart();
        	} else if (lParameterNumber == 4) {
        		uint8_t check = 1;
        		uint8_t ipdata[12];

        		printf("ips %s | %s | %s\n", ipaddr, ipdst, subnet);
        		if (prvCheckValidIPAddress(ipaddr, ipdata) != 4) check = 0;
        		if (prvCheckValidIPAddress(ipdst, ipdata + 4) != 4) check = 0;
        		if (prvCheckValidIPAddress(subnet, ipdata + 8) != 4) check = 0;

        		if (check) {
        			set_net_conf(ipdata, ipdata + 4, ipdata + 8);
        			sprintf( pcWriteBuffer, "Success\n" );
        		} else
        			sprintf( pcWriteBuffer, "Invalid parameters\n" );
			} else
				sprintf( pcWriteBuffer, "Wrong number of parameters\r\n" );


            /* There is no more data to return, so this time set xReturn to
            pdFALSE. */
            xReturn = pdFALSE;

            /* Start over the next time this command is executed. */
            lParameterNumber = 0;
        }
    }

    return xReturn;
}

void vRegisterCLICommands( void )
{
	/* Register all the command line commands defined immediately above. */
	FreeRTOS_CLIRegisterCommand( &xTaskStats );
	FreeRTOS_CLIRegisterCommand( &xIPStats );
	FreeRTOS_CLIRegisterCommand( &xRebootDevice );
	FreeRTOS_CLIRegisterCommand( &xBootlog );
	FreeRTOS_CLIRegisterCommand( &xI2CControl );
	FreeRTOS_CLIRegisterCommand( &xChCtrl );
	FreeRTOS_CLIRegisterCommand( &xDACControl );
	FreeRTOS_CLIRegisterCommand( &xEEPromR );
	FreeRTOS_CLIRegisterCommand( &xEEPromW );
	FreeRTOS_CLIRegisterCommand( &xStatusControl );
	FreeRTOS_CLIRegisterCommand( &xStartCControl );
	FreeRTOS_CLIRegisterCommand( &xStopCControl );
	FreeRTOS_CLIRegisterCommand( &xRestartCControl );
	FreeRTOS_CLIRegisterCommand( &xCalibrateChannel );
	FreeRTOS_CLIRegisterCommand( &xCalibrateChannelManual );
	FreeRTOS_CLIRegisterCommand( &xBIASChannel );
	FreeRTOS_CLIRegisterCommand( &xADCReadout );
	FreeRTOS_CLIRegisterCommand( &xCALCPWRCommand );
	FreeRTOS_CLIRegisterCommand( &xCALCCLRCommand );
	FreeRTOS_CLIRegisterCommand( &xCalibrateBias );
	FreeRTOS_CLIRegisterCommand( &xClearAlertCMD );
	FreeRTOS_CLIRegisterCommand( &xMACChange );
}

void vCommandConsoleTask( void *pvParameters )
{
	int16_t cRxedChar, cInputIndex = 0;
	BaseType_t xMoreDataToFollow;
	/* The input and output buffers are declared static to keep them off the stack. */
	static int8_t pcOutputString[ MAX_OUTPUT_LENGTH ], pcInputString[ MAX_INPUT_LENGTH ];
    puts((char *) pcWelcomeMessage);

    for( ;; )
    {
    	if (xQueueReceive(_xRxQueue, &cRxedChar, portMAX_DELAY))
    	{
            if(cRxedChar == '\n' || cRxedChar == '\r')
            {
                /* A newline character was received, so the input command string is
                complete and can be processed.  Transmit a line separator, just to
                make the output easier to read. */
                // FreeRTOS_write( xConsole, "\r\n", strlen( "\r\n" );
//            	printf("\n\r");

                /* The command interpreter is called repeatedly until it returns
                pdFALSE.  See the "Implementing a command" documentation for an
                exaplanation of why this is. */
                do
                {
                    /* Send the command string to the command interpreter.  Any
                    output generated by the command interpreter will be placed in the
                    pcOutputString buffer. */
                    xMoreDataToFollow = FreeRTOS_CLIProcessCommand
							  (
								  (char *) pcInputString,   /* The command string.*/
								  (char *) pcOutputString,  /* The output buffer. */
								  MAX_OUTPUT_LENGTH/* The size of the output buffer. */
							  );

                    /* Write the output generated by the command interpreter to the
                    console. */
                    // FreeRTOS_write( xConsole, pcOutputString,  );
                    pcOutputString[strlen( pcOutputString )] = '\0';
                    printf("%s", (char *) pcOutputString);

                } while( xMoreDataToFollow != pdFALSE );

                /* All the strings generated by the input command have been sent.
                Processing of the command is complete.  Clear the input string ready
                to receive the next command. */
                cInputIndex = 0;
                memset( pcInputString, 0x00, MAX_INPUT_LENGTH );
            }
            else
            {
                /* The if() clause performs the processing after a newline character
                is received.  This else clause performs the processing if any other
                character is received. */

                if( cRxedChar == '\r' )
                {
                    /* Ignore carriage returns. */
                }
                else if( cRxedChar == '\b' )
                {
                    /* Backspace was pressed.  Erase the last character in the input
                    buffer - if there are any. */
                    if( cInputIndex > 0 )
                    {
                        cInputIndex--;
                        pcInputString[ cInputIndex ] = '\0';
                    }
                }
                else
                {
                    /* A character was entered.  It was not a new line, backspace
                    or carriage return, so it is accepted as part of the input and
                    placed into the input buffer.  When a \n is entered the complete
                    string will be passed to the command interpreter. */
                    if( cInputIndex < MAX_INPUT_LENGTH )
                    {
                        pcInputString[ cInputIndex ] = cRxedChar;
                        cInputIndex++;
                    }
                }
            }
    	}
    }
}


