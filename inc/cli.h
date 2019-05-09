/*
 * cli.h
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */

#ifndef CLI_H_
#define CLI_H_

uint8_t prvCheckValidIPAddress(uint8_t * ipstr, uint8_t * result);
void vRegisterCLICommands( void );
void vCommandConsoleTask( void *pvParameters );

BaseType_t prvTaskStatsCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvRebootCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvNetworkConfigCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvBootlogCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvI2CCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCtrlCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
#endif /* CLI_H_ */
