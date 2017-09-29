/*
 * cli.c
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */
#include "config.h"
#include "cli.h"
#include "network.h"

#include "FreeRTOS_CLI.h"

#define MAX_INPUT_LENGTH    50
#define MAX_OUTPUT_LENGTH   100

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
static const CLI_Command_Definition_t xRebootDevice =
{
	"reboot", /* The command string to type. */
	"reboot:\r\n Reboots the device\r\n",
	prvRebootCommand, /* The function to run. */
	0 /* No parameters are expected. */
};

/* Structure that defines the "task-stats" command line command. */
static const CLI_Command_Definition_t xIPStats =
{
	"ifconfig", /* The command string to type. */
	"ifconfig:\r\n Displays network configuration info\r\n",
	prvNetworkConfigCommand, /* The function to run. */
	-1 /* No parameters are expected. */
};


BaseType_t prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	const char *const pcHeader = "Task State  Priority  Stack	#\r\n************************************************\r\n";

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
                    // FreeRTOS_write( xConsole, pcOutputString, strlen( pcOutputString ) );
                    puts((char *) pcOutputString);

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


