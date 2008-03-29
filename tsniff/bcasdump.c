#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"
#include "bcas_stream.h"

static guint8 st_firmware[] =
#include "fw.inc"

static gint st_fx2id = 0;
static gchar *st_bcas_filename = NULL;

static GOptionEntry st_options[] = {
	{ "fx2id", 'i', 0, G_OPTION_ARG_INT, &st_fx2id, "Set device id to ID [0]", "ID" },
	{ "bcas-filename", 'b', 0, G_OPTION_ARG_FILENAME, &st_bcas_filename, "Output B-CAS stream to FILENAME", "FILENAME" },
	{ NULL }
};

static gboolean st_is_running = TRUE;
static BCASStream *st_stream = NULL;

static void
parse_packet(const BCASPacket *packet, gboolean is_first_sync, gpointer user_data)
{
	gchar info[512];
	gint i;

	g_sprintf(info, "[%04x](%3d) ", packet->header, packet->len);
	for (i = 0; i < packet->len; ++i) {
		gchar x[4];
		g_sprintf(x, "%02x ", packet->payload[i]);
		g_strlcat(info, x, 511);
	}
	g_fprintf(stdout, "%s\n", info);
}

static void
transfer_bcas_cb(gpointer data, gint length, gpointer user_data)
{
	GIOChannel *io = (GIOChannel *)user_data;
	GError *error = NULL;
	gsize written;

	g_message("transfer_bcas_cb: %d bytes recieved", length);
	bcas_stream_push(st_stream, data, length, parse_packet, NULL);
	if (io) {
		g_io_channel_write_chars(io, data, length, &written, &error);
	}
}

static void
sighandler(int signum)
{
	st_is_running = FALSE;
}

int
main(int argc, char **argv)
{
	GOptionContext *context;
	cusbfx2_handle *device = NULL;
	cusbfx2_transfer *transfer_ts = NULL;
	cusbfx2_transfer *transfer_bcas = NULL;
	GIOChannel *io_bcas = NULL;
	GError *error = NULL;
	struct sigaction sigact;

	context = g_option_context_new("- CUSBFX2 B-CAS Stream dump");
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, st_options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		return 1;
	}

	g_log_set_handler(NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
					  g_log_default_handler, NULL);

	if (argc > 1) {
		gchar buf[512];
		gsize readed;
		io_bcas = g_io_channel_new_file(argv[1], "r", &error);
		if (!io_bcas) {
			return 1;
		}
		g_io_channel_set_encoding(io_bcas, NULL, &error);
		st_stream = bcas_stream_new();
		while (g_io_channel_read_chars(io_bcas, buf, 512, &readed, &error) == G_IO_STATUS_NORMAL) {
			bcas_stream_push(st_stream, buf, readed, parse_packet, NULL);
		}
		return 1;
	}

	/* initialize */
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);

	st_stream = bcas_stream_new();

	cusbfx2_init();

	device = cusbfx2_open(st_fx2id, st_firmware, "FX2_FIFO");
	if (!device) {
		goto quit;
	}

	capsts_exec_cmd(CMD_PORT_CFG, 0x00, PIO_START);
	capsts_exec_cmd(CMD_MODE_IDLE);
	capsts_exec_cmd(CMD_IFCONFIG, 0xE3);
	capsts_exec_cmd_queue(device);

	if (st_bcas_filename) {
		io_bcas = g_io_channel_new_file(st_bcas_filename, "w", &error);
		if (!io_bcas) {
			goto quit;
		}
		g_io_channel_set_encoding(io_bcas, NULL, &error);
	}

	transfer_bcas = cusbfx2_init_bulk_transfer(device, "B-CAS", ENDPOINT_BCAS_IN,
											   512, 1, transfer_bcas_cb, io_bcas);
	if (!transfer_bcas) {
		goto quit;
	}

	capsts_exec_cmd(CMD_EP4IN_START);
	capsts_exec_cmd(CMD_PORT_WRITE, PIO_START);
	capsts_exec_cmd_queue(device);

	/* dump */
	while (st_is_running) {
		if (cusbfx2_poll() < 0)
			break;
	}

	/* finalize */
 quit:
	if (transfer_bcas) {
		capsts_exec_cmd(CMD_EP4IN_STOP);
		capsts_exec_cmd(CMD_MODE_IDLE);
		capsts_exec_cmd_queue(device);
	}
	cusbfx2_free_transfer(transfer_bcas);
	g_io_channel_close(io_bcas);
	cusbfx2_close(device);

	cusbfx2_exit();

	return 0;
}
