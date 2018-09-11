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
BaseType_t prvDacCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCHSTATUSCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvEEPROMRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvEEPROMWCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvStartCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvStopCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvResetIntCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCalibrateChannelCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvBIASCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvADCCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCALPWRCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvClearCalibrationCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCalibrateChannelManualCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvCalibrateBIASCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvClearAlert( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvNetworkMACChange( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvBootloaderCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvIntCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvSWInfoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvINTCALCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
BaseType_t prvIntVCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );

#endif /* CLI_H_ */
