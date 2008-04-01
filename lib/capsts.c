#include <string.h>
#include <stdarg.h>
#define G_LOG_DOMAIN "capsts"
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"

/* CUSBFX2のCAPSTSファームウェア */
static guint8 st_firmware[] =
#include "fw.inc"
#define FIRMWARE_ID "FX2_FIFO_ATTY01"

static GByteArray *st_cmd_queue = NULL; /* CAPSTSファームウェアコマンドの送信キュー */

static GArray *st_ir_cmd_queue = NULL; /* IRコマンドの送信キュー */
static gint st_ir_base = 0;		/** IRのベースチャンネル */

/**
 * CUSBFX2を開いてハンドルを返す。
 *
 * 必要なファームウェアがCUSBFX2にロードされていなければ、
 * ファームウェアをアップロードした後、再検出した CUSBFX2 のハンドルを返す。
 *
 * @param[in]	fx2id	開くCUSBFX2のID
 * @param[in]	is_force_load	TRUEなら強制的にファームウェアをロードする
 */
cusbfx2_handle *
capsts_open(gint fx2id, gboolean is_force_load)
{
	cusbfx2_handle *device;
	device = cusbfx2_open(fx2id, st_firmware, FIRMWARE_ID, is_force_load);
	if (!device) {
		g_critical("Couldn't open CUSBFX2 device");
	}
	return device;
}

/**
 * CAPSTSファームウェアへのコマンドをキューに追加する。
 */
void
capsts_cmd_push(guint8 cmd, ...)
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

/**
 * CAPSTSファームウェアへのコマンドキューを実際に送信する。
 */
gboolean
capsts_cmd_commit(cusbfx2_handle *device)
{
	gboolean result = TRUE;

	g_assert(st_cmd_queue);

	if (cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, st_cmd_queue->data, st_cmd_queue->len) != st_cmd_queue->len) {
		g_warning("capsts_cmd_commit: cusbfx2_bulk_transfer failed");
		result = FALSE;
	}

	g_byte_array_free(st_cmd_queue, TRUE);
	st_cmd_queue = NULL;

	return result;
}

/**
 * IRのベースチャンネルを変更する。
 * デフォルトは 1 。
 * @param base	ベースチャンネル(1〜3)
 */
void
capsts_set_ir_base(gint base)
{
	st_ir_base = (CLAMP(base, 1, 3) - 1) * 0x80;
}

/**
 * IRコマンド送信キューに新規コマンドを追加する。
 *
 * @param cmd	追加するコマンド
 */
void
capsts_ir_cmd_push(CapStsIrCommand cmd)
{
	/* まだキューが無ければ作成 */
    if (!st_ir_cmd_queue) {
		st_ir_cmd_queue = g_array_new(TRUE, TRUE, sizeof(CapStsIrCommand));
    }

	g_array_append_val(st_ir_cmd_queue, cmd);
}

/**
 * IRコマンド送信キューを実際に送信する。
 *
 * @param device
 */
gboolean
capsts_ir_cmd_commit(cusbfx2_handle *device)
{
	guint i;
	guint8 cmds[3] = { CMD_IR_CODE };

	for (i = 0; i < st_ir_cmd_queue->len; ++i) {
		guint16 cmd = g_array_index(st_ir_cmd_queue, CapStsIrCommand, i);

		if (cmd == IR_CMD_3DIGIT_INPUT) {
			// wait == 300msec needed ?
		}

		/* コマンドの信号を立ち上げる */
		cmds[1] = (cmd + st_ir_base) & 0xFF;
		cmds[2] = ((cmd + st_ir_base) >> 8) & 0xFF;
		if (cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3) != 3) {
			g_warning("Couldn't send IR command");
			return FALSE;
		}

		g_usleep(600 * 1000);

		/* コマンドの信号を立ち下げる */
		cmds[1] = cmds[2] = 0;
		if (cusbfx2_bulk_transfer(device, ENDPOINT_CMD_OUT, cmds, 3) != 3) {
			g_warning("Couldn't send IR command");
			return FALSE;
		}

		g_usleep(100 * 1000);
	}

	/* 送信したキューを削除 */
	if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
	st_ir_cmd_queue = NULL;
}

/**
 * チューナーの入力ソースとチャンネルを変更する。
 *
 * @param device
 * @param source	入力ソース -- TUNER_SOURCE_MAX の場合は変更しない
 * @param chnannel	1ボタンチャンネル(1〜12) -- 範囲外の場合は変更しない
 * @param three_channel	3桁チャンネル(000〜999) -- BS/CSの場合のみ
 */
gboolean
capsts_adjust_tuner_channel(cusbfx2_handle *device, CapStsTunerSource source, gint channel, const gchar *three_channel)
{
	/* まず入力ソースを切り替える */
	switch (source) {
	case TUNER_SOURCE_TERESTRIAL:
		capsts_ir_cmd_push(IR_CMD_DIGITAL_TERESTRIAL1);
		break;
	case TUNER_SOURCE_BS:
		capsts_ir_cmd_push(IR_CMD_FORMAT_BS);
		break;
	case TUNER_SOURCE_CS:
		capsts_ir_cmd_push(IR_CMD_FORMAT_CS);
		break;
	default:
		/* 現在の入力ソースのまま切り替えない */
		break;
	}

	/* 入力がBS/CSかつ3桁チャンネルが指定されていれば、3桁チャンネル入力によってチャンネルを変更 */
	if (three_channel && source != TUNER_SOURCE_TERESTRIAL) {
		capsts_ir_cmd_push(IR_CMD_3DIGIT_INPUT);

		if (strlen(three_channel) != 3) {
			g_critical("Invalid channel format (must be three digit)");
			if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
			return FALSE;
		} else {
			const gchar *p;
			for (p = three_channel; *p; ++p) {
				capsts_ir_cmd_push(IR_CMD_0 + CLAMP(g_ascii_digit_value(*p), 0, 9));
			}
		}
	} else if (channel >= 1 && channel <= 12) {
		/* 通常のチャンネルが指定されていれば、変更 */
		static CapStsIrCommand cmd[] = { IR_CMD_1, IR_CMD_2, IR_CMD_3, IR_CMD_4, IR_CMD_5, IR_CMD_6,
										 IR_CMD_7, IR_CMD_8, IR_CMD_9, IR_CMD_0, IR_CMD_11, IR_CMD_12 };
		capsts_ir_cmd_push(cmd[CLAMP(channel, 1, 12) - 1]);
	}

	if (st_ir_cmd_queue->len > 0) {
		if (!capsts_ir_cmd_commit(device)) {
			if (st_ir_cmd_queue) g_array_free(st_ir_cmd_queue, TRUE);
			return FALSE;
		}

		/* チューナー側での切り替えを待つ */
		g_usleep(2500 * 1000);
	}
}
