/*
 * cli.h
 *
 *  Created on: Aug 4, 2017
 *      Author: wizath
 */

#ifndef CLI_H_
#define CLI_H_

#include "ucli.h"
#include "config.h"

void cli_init(void);
void vCommandConsoleTask( void *pvParameters );

#endif /* CLI_H_ */
