#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"

static guint8 st_firmware[] =
#include "fw.inc"


static gint st_fx2id = 0;
static gchar *st_ts_filename = NULL;
static gchar *st_bcas_filename = NULL;
static gint st_buffer_length = 16384;
static gint st_buffer_count = 16;

static GOptionEntry st_options[] = {
	{ "fx2id", 'i', 0, G_OPTION_ARG_INT, &st_fx2id, "Set device id to ID (Default:0)", "ID" },
	{ "ts-filename", 't', 0, G_OPTION_ARG_FILENAME, &st_ts_filename, "Output MPEG2-TS to FILENAME", "FILENAME" },
	{ "bcas-filename", 'b', 0, G_OPTION_ARG_FILENAME, &st_bcas_filename, "Output B-CAS stream to FILENAME", "FILENAME" },
	{ NULL }
};

static void
transfer_ts_cb(gpointer data, gint length, gpointer user_data)
{
	GIOChannel *io = (GIOChannel *)user_data;
	GError *error = NULL;
	gsize written;

	g_io_channel_write_chars(io, data, length, &written, &error);
}

static void
transfer_bcas_cb(gpointer data, gint length, gpointer user_data)
{
	GIOChannel *io = (GIOChannel *)user_data;
	GError *error = NULL;
	gsize written;

	g_io_channel_write_chars(io, data, length, &written, &error);
}

static void
rec(void)
{
	cusbfx2_handle *device = NULL;
	cusbfx2_transfer *transfer_ts = NULL;
	cusbfx2_transfer *transfer_bcas = NULL;
	GIOChannel *io_ts = NULL, *io_bcas = NULL;
	GError *error = NULL;
	GTimer *timer; 

	device = cusbfx2_open(st_fx2id, st_firmware, "FX2_FIFO");
	if (!device) {
		return;
	}

	capsts_exec_cmd(CMD_PORT_CFG, 0x00, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
	capsts_exec_cmd(CMD_MODE_IDLE);
	capsts_exec_cmd(CMD_IFCONFIG, 0xE3);
	capsts_exec_cmd_queue(device);

	if (st_ts_filename) {
		io_ts = g_io_channel_new_file(st_ts_filename, "w", &error);
		if (!io_ts) {
			goto quit;
		}
		g_io_channel_set_encoding(io_ts, NULL, &error);

		transfer_ts = cusbfx2_init_bulk_transfer(device, "MPEG-TS", ENDPOINT_TS_IN,
												 st_buffer_length, st_buffer_count,
												 transfer_ts_cb, io_ts);
		if (!transfer_ts) {
			goto quit;
		}
		capsts_exec_cmd(CMD_EP6IN_START);
	}

	if (st_bcas_filename) {
		io_bcas = g_io_channel_new_file(st_bcas_filename, "w", &error);
		if (!io_bcas) {
			goto quit;
		}
		g_io_channel_set_encoding(io_bcas, NULL, &error);

		transfer_bcas = cusbfx2_init_bulk_transfer(device, "B-CAS", ENDPOINT_BCAS_IN,
												   512, 1, transfer_bcas_cb, io_bcas);
		if (!transfer_bcas) {
			goto quit;
		}
		capsts_exec_cmd(CMD_EP4IN_START);
	}

	capsts_exec_cmd(CMD_PORT_WRITE, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
	capsts_exec_cmd_queue(device);

	timer = g_timer_new();
	for (;;) {
		cusbfx2_poll();
		if (g_timer_elapsed(timer, NULL) > 15.0) {
			break;
		}
	}
	g_timer_destroy(timer);

	capsts_exec_cmd(CMD_EP6IN_STOP);
	if (st_bcas_filename) capsts_exec_cmd(CMD_EP4IN_STOP);
	capsts_exec_cmd(CMD_MODE_IDLE);
	capsts_exec_cmd_queue(device);

 quit:
	cusbfx2_free_transfer(transfer_ts);
	cusbfx2_free_transfer(transfer_bcas);
	g_io_channel_close(io_ts);
	g_io_channel_close(io_bcas);
	cusbfx2_close(device);
}

int
main(int argc, char **argv)
{
	GError *error;
	GOptionContext *context;

	context = g_option_context_new("- CUSBFX2 MPEG2-TS Recorder");
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, st_options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		return 1;
	}

	g_log_set_handler(NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
					  g_log_default_handler, NULL);

	cusbfx2_init();

	rec();

	cusbfx2_exit();

	return 0;
}
