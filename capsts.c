#include <stdarg.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"

static GByteArray *st_cmd_queue = NULL;
static GArray *st_ir_cmd_queue = NULL;


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

void
capsts_ir_cmd_append(guint16 cmd)
{
    if (!st_ir_cmd_queue) {
		st_ir_cmd_queue = g_array_new(TRUE, TRUE, sizeof(guint16));
    }

	g_array_append_val(st_ir_cmd_queue, cmd);
}

void
capsts_ir_cmd_send(cusbfx2_handle *device)
{
	guint i;
	guint8 cmds[3] = { CMD_IR_CODE };

	g_assert(st_ir_cmd_queue);

	for (i = 0; i < st_ir_cmd_queue->len; ++i) {
		guint16 cmd = g_array_index(st_ir_cmd_queue, guint16, i);

		cmds[1] = cmd & 0xFF;
		cmds[2] = (cmd >> 8) & 0xFF;
		cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3);

		if (cmd == IR_CMD_3DIGIT_INPUT) {
			// wait == 300msec needed ?
		}
		g_usleep(600 * 1000);

		cmds[1] = cmds[2] = 0;
		cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3);

		g_usleep(100 * 1000);
	}

	g_array_free(st_ir_cmd_queue, TRUE);
	st_ir_cmd_queue = NULL;
}
