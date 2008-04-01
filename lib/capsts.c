#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"

static GByteArray *st_cmd_queue = NULL;

static GArray *st_ir_cmd_queue = NULL;
static gint st_ir_base = 0;

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

/**
 * IR�Υ١��������ͥ���ѹ����롣
 * �ǥե���Ȥ� 1 ��
 * @param base	�١��������ͥ�(1��3)
 */
void
capsts_set_ir_base(gint base)
{
	st_ir_base = (CLAMP(base, 1, 3) - 1) * 0x80;
}

/**
 * IR���ޥ���������塼�˿������ޥ�ɤ��ɲä��롣
 *
 * @param cmd	�ɲä��륳�ޥ��
 */
void
capsts_ir_cmd_append(CapStsIrCommand cmd)
{
	/* �ޤ����塼��̵����к��� */
    if (!st_ir_cmd_queue) {
		st_ir_cmd_queue = g_array_new(TRUE, TRUE, sizeof(CapStsIrCommand));
    }

	g_array_append_val(st_ir_cmd_queue, cmd);
}

/**
 * IR���ޥ���������塼��ºݤ��������롣
 *
 * @param device
 */
gboolean
capsts_ir_cmd_send(cusbfx2_handle *device)
{
	guint i;
	guint8 cmds[3] = { CMD_IR_CODE };

	for (i = 0; i < st_ir_cmd_queue->len; ++i) {
		guint16 cmd = g_array_index(st_ir_cmd_queue, CapStsIrCommand, i);

		if (cmd == IR_CMD_3DIGIT_INPUT) {
			// wait == 300msec needed ?
		}

		/* ���ޥ�ɤο����Ω���夲�� */
		cmds[1] = (cmd + st_ir_base) & 0xFF;
		cmds[2] = ((cmd + st_ir_base) >> 8) & 0xFF;
		if (cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3) != 3) {
			g_warning("Couldn't send IR command");
			return FALSE;
		}

		g_usleep(600 * 1000);

		/* ���ޥ�ɤο����Ω�������� */
		cmds[1] = cmds[2] = 0;
		if (cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3) != 3) {
			g_warning("Couldn't send IR command");
			return FALSE;
		}

		g_usleep(100 * 1000);
	}

	/* �����������塼���� */
	if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
	st_ir_cmd_queue = NULL;
}

/**
 * ���塼�ʡ������ϥ������ȥ����ͥ���ѹ����롣
 *
 * @param device
 * @param source	���ϥ����� -- TUNER_SOURCE_MAX �ξ����ѹ����ʤ�
 * @param chnannel	1�ܥ�������ͥ�(1��12) -- �ϰϳ��ξ����ѹ����ʤ�
 * @param three_channel	3������ͥ�(000��999) -- BS/CS�ξ��Τ�
 */
gboolean
capsts_adjust_tuner_channel(cusbfx2_handle *device, CapStsTunerSource source, gint channel, const gchar *three_channel)
{
	/* �ޤ����ϥ��������ڤ��ؤ��� */
	switch (source) {
	case TUNER_SOURCE_TERESTRIAL:
		capsts_ir_cmd_append(IR_CMD_DIGITAL_TERESTRIAL1);
		break;
	case TUNER_SOURCE_BS:
		capsts_ir_cmd_append(IR_CMD_FORMAT_BS);
		break;
	case TUNER_SOURCE_CS:
		capsts_ir_cmd_append(IR_CMD_FORMAT_CS);
		break;
	default:
		/* ���ߤ����ϥ������Τޤ��ڤ��ؤ��ʤ� */
		break;
	}

	/* ���Ϥ�BS/CS����3������ͥ뤬���ꤵ��Ƥ���С�3������ͥ����Ϥˤ�äƥ����ͥ���ѹ� */
	if (three_channel && source != TUNER_SOURCE_TERESTRIAL) {
		capsts_ir_cmd_append(IR_CMD_3DIGIT_INPUT);

		if (strlen(three_channel) != 3) {
			g_critical("Invalid channel format (must be three digit)");
			if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
			return FALSE;
		} else {
			const gchar *p;
			for (p = three_channel; *p; ++p) {
				capsts_ir_cmd_append(IR_CMD_0 + CLAMP(g_ascii_digit_value(*p), 0, 9));
			}
		}
	} else if (channel >= 1 && channel <= 12) {
		/* �̾�Υ����ͥ뤬���ꤵ��Ƥ���С��ѹ� */
		static CapStsIrCommand cmd[] = { IR_CMD_1, IR_CMD_2, IR_CMD_3, IR_CMD_4, IR_CMD_5, IR_CMD_6,
										 IR_CMD_7, IR_CMD_8, IR_CMD_9, IR_CMD_0, IR_CMD_11, IR_CMD_12 };
		capsts_ir_cmd_append(cmd[CLAMP(channel, 1, 12) - 1]);
	}

	if (st_ir_cmd_queue->len > 0) {
		if (!capsts_ir_cmd_send(device)) {
			if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
			return FALSE;
		}

		/* ���塼�ʡ�¦�Ǥ��ڤ��ؤ����Ԥ� */
		g_usleep(2500 * 1000);
	}
}
