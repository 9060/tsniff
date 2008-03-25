#include <stdarg.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"

static GByteArray *st_cmd_queue = NULL;

void
capsts_exec_cmd(guint8 cmd, ...)
{
    va_list ap;

    if (!st_cmd_queue) {
		st_cmd_queue = g_byte_array_sized_new(128);
    }

	g_byte_array_append(st_cmd_queue, &cmd, 1);

    va_start(ap, cmd);
    switch (cmd) {
		guint8 arg;
		guint8 len;
		guint8 *data;

    case CMD_IR_WBUF:			/* special */
		arg = va_arg(ap, gint);
		g_byte_array_append(st_cmd_queue, &arg, 1);
		len = va_arg(ap, gint);
		g_byte_array_append(st_cmd_queue, &len, 1);
		data = va_arg(ap, guint8 *);
		g_byte_array_append(st_cmd_queue, data, len);
		break;

    case CMD_REG_WRITE:			/* 2 arguments */
    case CMD_IR_CODE:
		arg = va_arg(ap, gint);
		g_byte_array_append(st_cmd_queue, &arg, 1);
		/* FALLTHROUGH */

    case CMD_PORT_CFG:			/* 1 argument */
    case CMD_REG_READ:
    case CMD_PORT_WRITE:
    case CMD_IFCONFIG:
    case CMD_IR_RBUF:
		arg = va_arg(ap, gint);
		g_byte_array_append(st_cmd_queue, &arg, 1);
		/* FALLTHROUGH */

    case CMD_EP6IN_START:		/* no arguments */
    case CMD_EP6IN_STOP:
    case CMD_EP2OUT_START:
    case CMD_EP2OUT_STOP:
    case CMD_PORT_READ:
    case CMD_MODE_IDLE:
    case CMD_EP4IN_START:
    case CMD_EP4IN_STOP:
		break;

	default:
		g_assert_not_reached();
    }

    va_end(ap);
}

void
capsts_exec_cmd_queue(cusbfx2_handle *device)
{
	g_assert(st_cmd_queue);

	cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, st_cmd_queue->data, st_cmd_queue->len);

	g_byte_array_free(st_cmd_queue, TRUE);
	st_cmd_queue = NULL;
}
