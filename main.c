#include <glib.h>
#include "cusbfx2.h"

static guint8 st_firmware[] =
#include "fw.inc"

#define CMD_EP6IN_START		0x50	//
#define	CMD_EP6IN_STOP		0x51	//
#define	CMD_EP2OUT_START	0x52	//
#define	CMD_EP2OUT_STOP		0x53	//
#define	CMD_PORT_CFG		0x54	//addr_mask, out_pins
#define	CMD_REG_READ		0x55	//addr	(return 1byte)
#define	CMD_REG_WRITE		0x56	//addr, value
#define	CMD_PORT_READ		0x57	//(return 1byte)
#define	CMD_PORT_WRITE		0x58	//value
#define	CMD_IFCONFIG		0x59	//value
#define	CMD_MODE_IDLE		0x5a
#define CMD_EP4IN_START		0x5b	//
#define	CMD_EP4IN_STOP		0x5c	//
#define	CMD_IR_CODE			0x5d	//val_l, val_h (0x0000:RBUF capture 0xffff:BWUF output)
#define CMD_IR_WBUF			0x5e	//ofs(0 or 64 or 128 or 192), len(max 64), data, ....
#define	CMD_IR_RBUF			0x5f	//ofs(0 or 64 or 128 or 192) (return 64byte)
#define	PIO_START	0x20
#define	PIO_IR_OUT	0x10
#define	PIO_IR_IN	0x08
#define PIO_TS_BACK	0x04

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

	if (st_ts_filename) {
		io_ts = g_io_channel_new_file(st_ts_filename, "w", &error);
		if (!io_ts) {
			goto quit;
		}
		g_io_channel_set_encoding(io_ts, NULL, &error);

		transfer_ts = cusbfx2_init_bulk_transfer(device, 0x86, st_buffer_length, st_buffer_count,
												 transfer_ts_cb, io_ts);
		if (!transfer_ts) {
			goto quit;
		}
	}

	if (st_bcas_filename) {
		io_bcas = g_io_channel_new_file(st_bcas_filename, "w", &error);
		if (!io_bcas) {
			goto quit;
		}
		g_io_channel_set_encoding(io_bcas, NULL, &error);

		transfer_bcas = cusbfx2_init_bulk_transfer(device, 0x84, 512, 1, transfer_bcas_cb, io_bcas);
		if (!transfer_bcas) {
			goto quit;
		}
	}

	{
		guint8 cmd[] = { CMD_PORT_CFG, 0x00, PIO_START|PIO_IR_OUT|PIO_TS_BACK,
						 CMD_MODE_IDLE, CMD_IFCONFIG, 0xE3 };
		cusbfx2_bulk_transfer(device, 0x01, cmd, sizeof(cmd));
	}
	{
		guint8 cmd[] = { CMD_EP6IN_START, CMD_EP4IN_START, CMD_PORT_WRITE, PIO_START|PIO_IR_OUT|PIO_TS_BACK };
		cusbfx2_bulk_transfer(device, 0x01, cmd, sizeof(cmd));
	}

	timer = g_timer_new();
	for (;;) {
		cusbfx2_poll();
		if (g_timer_elapsed(timer, NULL) > 15.0) {
			break;
		}
	}
	g_timer_destroy(timer);

	{
		guint8 cmd[] = { CMD_EP6IN_STOP, CMD_EP4IN_STOP, CMD_MODE_IDLE };
		cusbfx2_bulk_transfer(device, 0x01, cmd, sizeof(cmd));
	}

 quit:
	if (transfer_ts) {
		//cusbfx2_cancel_transfer_sync(transfer_ts);
		cusbfx2_free_transfer(transfer_ts);
	}
	if (transfer_bcas) {
		//cusbfx2_cancel_transfer_sync(transfer_bcas);
		cusbfx2_free_transfer(transfer_bcas);
	}
	if (io_ts) g_io_channel_close(io_ts);
	if (io_bcas) g_io_channel_close(io_bcas);
	if (device) cusbfx2_close(device);
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
