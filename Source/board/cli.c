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

#include "FreeRTOS_CLI.h"

#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

extern xQueueHandle _xRxQueue;
extern channel_t channels[8];

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
	4 /* Dynamic number of parameters. */
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

extern TaskHandle_t xChannelTask;
extern TaskHandle_t xStatusTask;

BaseType_t prvStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	( void ) pcCommandString;
	( void ) xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	vTaskSuspend(xChannelTask);
	vTaskSuspend(xStatusTask);

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

	vTaskResume(xChannelTask);
	vTaskResume(xStatusTask);

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

	rf_clear_interlock();

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

				if (pwr_type == 1) {
					printf("Value1 for pwr %d = %d\n", pwr_cal, channels[channel].adc_raw_ch1);
					cal_in_pwr1 = pwr_cal;
					cal_in_val1 = channels[channel].adc_raw_ch1;
				} else if (pwr_type == 2) {
					printf("Value2 for pwr %d = %d\n", pwr_cal, channels[channel].adc_raw_ch1);
					cal_in_pwr2 = pwr_cal;
					cal_in_val2 = channels[channel].adc_raw_ch1;
				} else if (pwr_type == 3) {
					printf("Value1 for pwr %d = %d\n", pwr_cal, channels[channel].adc_raw_ch2);
					cal_rfl_pwr1 = pwr_cal;
					cal_rfl_val1 = channels[channel].adc_raw_ch2;
				} else if (pwr_type == 4) {
					printf("Value2 for pwr %d = %d\n", pwr_cal, channels[channel].adc_raw_ch2);
					cal_rfl_pwr2 = pwr_cal;
					cal_rfl_val2 = channels[channel].adc_raw_ch2;
				}

				if (cal_in_val1 != 0 && cal_in_val2 != 0)
				{
					printf("Calculating points for %d -> %d\n", cal_in_pwr1, cal_in_pwr2);
					uint16_t a = ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2));
					uint16_t b = (cal_in_val1 - ((cal_in_val1 - cal_in_val2) / (cal_in_pwr1 - cal_in_pwr2)) * cal_in_pwr1);
					printf("INPWR A = %d B = %d\n", a, b);

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
				printf("ADCs %d %d\n", channels[channel].adc_raw_ch1, channels[channel].adc_raw_ch2);
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
	static uint8_t chan;
	static int16_t startval;
	static uint8_t step;

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
			if (lParameterNumber == 2) chan = atoi((char*) pcParameter);
			if (lParameterNumber == 3) startval = atoi((char*) pcParameter);
			if (lParameterNumber == 4) step = atoi((char*) pcParameter);

			xReturn = pdTRUE;
			lParameterNumber++;

			sprintf(pcWriteBuffer, "\r");
		} else {
			printf("Execuring cal command, ch %d\n\r", channel);
			/* There is no more data to return, so this time set xReturn to
			   pdFALSE. */
			int16_t dacval = 0;

			if (startval > 0 && startval < 1700)
				dacval = startval;
			else
				dacval = 1700;

			if (step == 0) step = 1;

			if (channel < 8) {

				vTaskSuspend(xChannelTask);
				vTaskDelay(100);
				rf_channel_enable_procedure(channel);
				vTaskDelay(100);
				rf_clear_interlock();

				while (dacval > 0) {

					printf("Trying channel %d value %d\n", channel, dacval);

					if (lock_take(I2C_LOCK, portMAX_DELAY))
					{
						i2c_mux_select(channel);
						if (chan == 0)
							i2c_dual_dac_set(0, dacval);
						else if (chan == 1)
							i2c_dual_dac_set(1, dacval);

						lock_free(I2C_LOCK);
					}

					vTaskDelay(200);

					if (lock_take(SPI_LOCK, portMAX_DELAY)) {
						led_bar_write(rf_channels_read_sigon(), (rf_channels_read_ovl()) & rf_channels_read_sigon(), (rf_channels_read_user()) & rf_channels_read_sigon());
						lock_free(SPI_LOCK);
					}

					uint8_t val = 0;
					if (chan == 0)
						val = rf_channels_read_ovl();
					else if (chan == 1)
						val = rf_channels_read_user();

					if ((val >> channel) & 0x01) {
						printf("Interlock ON at %d\n", dacval);

						if (lock_take(I2C_LOCK, portMAX_DELAY))
						{
							i2c_mux_select(channel);

							if (chan == 0) {
								rf_channel_save_dac(DAC1_EEPROM_ADDRESS, dacval);
								channels[channel].dac1_value = dacval;
							} else if (chan == 1) {
								rf_channel_save_dac(DAC2_EEPROM_ADDRESS, dacval);
								channels[channel].dac2_value = dacval;
							}

							lock_free(I2C_LOCK);
						}

						break;
					}

					vTaskDelay(200);
					dacval -= step;
				}

				rf_channel_disable_procedure(channel);
				vTaskDelay(100);

				if (lock_take(SPI_LOCK, portMAX_DELAY)) {
					led_bar_write(0, 0, 0);
					lock_free(SPI_LOCK);
				}

				vTaskResume(xChannelTask);
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

extern volatile uint8_t channel_mask;

BaseType_t prvCtrlCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	int8_t *pcParameter;
	BaseType_t lParameterStringLength, xReturn;

	static uint8_t channels;

	/* Note that the use of the static parameter means this function is not reentrant. */
	static BaseType_t lParameterNumber = 0;

	if( lParameterNumber == 0 )
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
	FreeRTOS_CLIRegisterCommand( &xBIASChannel );
	FreeRTOS_CLIRegisterCommand( &xADCReadout );
	FreeRTOS_CLIRegisterCommand( &xCALCPWRCommand );
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


