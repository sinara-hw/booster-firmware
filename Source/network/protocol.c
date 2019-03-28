/*
 * protocol.c
 *
 *  Created on: Sep 29, 2017
 *      Author: wizath
 */

#include "protocol.h"
#include "network.h"
#include "server.h"
#include "socket.h"

user_data_t user_data = {
    .ipsrc = 0,
    .ipsrc_port = 0,
};

size_t SCPI_Write(scpi_t * context, const char * data, size_t len)
{
	if (context->user_context != NULL) {

		user_data_t * u = (user_data_t *) (context->user_context);
		if (u->ipsrc) {
			printf("Sending to socket %d\r\n", u->socket);
			return send(0, (uint8_t *) data, len);
		}

	}
	return 0;
}

scpi_result_t SCPI_Flush(scpi_t * context)
{
	context->output_count = 0; // flush output to avoid newlines
    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;
    /* BEEP */
    uint8_t errmsg[64] = { 0 };
    uint8_t len = sprintf((char*) errmsg, "[scpi] **ERROR: %ld, \"%s\"\r\n", (int32_t) err, SCPI_ErrorTranslate(err));

    if (err != 0) {
    	user_data_t * u = (user_data_t *) (context->user_context);
    	if (u->ipsrc) send(0, (uint8_t *) errmsg, len);
        /* New error */
        /* Beep */
        /* Error LED ON */
    } else {
        /* No more errors in the queue */
        /* Error LED OFF */
    }

    /* Clear remaining errors */
    SCPI_ErrorClear(context);
    return 0;
}


scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    char b[16];

//    if (SCPI_CTRL_SRQ == ctrl) {
//        printf("[scpi] **SRQ: 0x%X (%d)\r\n", val, val);
//    } else {
//        printf("[scpi] **CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
//    }

//    if (context->user_context != NULL) {
//        user_data_t * u = (user_data_t *) (context->user_context);
//        if (u->ipsrc) {
//            snprintf(b, sizeof (b), "SRQ%d\r\n", val);
//            return sendto(0, (uint8_t *) b, strlen(b), u->ipsrc, u->ipsrc_port);
//        }
//    }
    return SCPI_RES_OK;
}

/* Reset function */
scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;
    NVIC_SystemReset();
    return SCPI_RES_OK;
}

void scpi_init(void)
{
	SCPI_Init(&scpi_context,
			  scpi_commands,
			  &scpi_interface,
			  scpi_units_def,
			  BOARD_MANUFACTURER, BOARD_NAME, BOARD_REV, BOARD_YEAR,
			  scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
			  scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

	scpi_context.user_context = &user_data;
}
