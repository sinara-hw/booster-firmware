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
    .ipsrc = NULL,
    .ipsrc_port = NULL,
};

size_t SCPI_Write(scpi_t * context, const char * data, size_t len)
{
	if (context->user_context != NULL) {
		user_data_t * u = (user_data_t *) (context->user_context);
		if (u->ipsrc) return sendto(0, (uint8_t *) data, len, u->ipsrc, u->ipsrc_port);
	}
	return 0;
}

scpi_result_t SCPI_Flush(scpi_t * context)
{
    if (context->user_context != NULL) {
//        user_data_t * u = (user_data_t *) (context->user_context);
		/* flush not implemented */
		return SCPI_RES_OK;
    }
    return SCPI_RES_OK;
}

int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;
    /* BEEP */
    printf("**ERROR: %ld, \"%s\"\r\n", (int32_t) err, SCPI_ErrorTranslate(err));
    if (err != 0) {
        /* New error */
        /* Beep */
        /* Error LED ON */
    } else {
        /* No more errors in the queue */
        /* Error LED OFF */
    }
    return 0;
}

scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    char b[16];

    if (SCPI_CTRL_SRQ == ctrl) {
        printf("**SRQ: 0x%X (%d)\r\n", val, val);
    } else {
        printf("**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }

    if (context->user_context != NULL) {
        user_data_t * u = (user_data_t *) (context->user_context);
        if (u->ipsrc) {
            snprintf(b, sizeof (b), "SRQ%d\r\n", val);
            return sendto(0, b, strlen(b), u->ipsrc, u->ipsrc_port);
        }
    }
    return SCPI_RES_OK;
}

scpi_result_t SCPI_Reset(scpi_t * context) {
    (void) context;
    printf("**Reset\r\n");
    return SCPI_RES_OK;
}

void scpi_init(void)
{
	SCPI_Init(&scpi_context,
			  scpi_commands,
			  &scpi_interface,
			  scpi_units_def,
			  SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
			  scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
			  scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

	scpi_context.user_context = &user_data;
}
