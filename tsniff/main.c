#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"
#include "arib_std_b25.h"
#include "b_cas_card.h"
#include "bcas_card_streaming.h"

/* Options
   -------------------------------------------------------------------------- */
static gint st_fx2_id = 0;		/* CUSBFX2のID */
static gboolean st_fx2_is_force_load = FALSE; /* CUSBFX2のファームウェアを強制的にロードする */
static gint st_fx2_ts_buffer_size = 16384;
static gint st_fx2_ts_buffer_count = 16;
static GOptionEntry st_fx2_options[] = {
	{ "fx2-id", 0, 0, G_OPTION_ARG_INT, &st_fx2_id,
	  "Find CUSBFX2 it has ID N [0]", "N" },
	{ "fx2-force-load", 0, 0, G_OPTION_ARG_NONE, &st_fx2_is_force_load,
	  "Force load the firmware [disabled]", NULL },
	{ "fx2-ts-buffer-size", 0, 0, G_OPTION_ARG_INT, &st_fx2_ts_buffer_size,
	  "Set TS transfer buffer size to N bytes [16384]", "N" },
	{ "fx2-ts-buffer-count", 0, 0, G_OPTION_ARG_INT, &st_fx2_ts_buffer_size,
	  "Set TS transfer buffer count to N [16]", "N" },
	{ NULL }
};

static gint st_ir_base = 0;
static gint st_ir_source = -1;
static gint st_ir_channel = -1;
static gchar *st_ir_three_channel = NULL;
static GOptionEntry st_ir_options[] = {
	{ "ir-base", 0, 0, G_OPTION_ARG_INT, &st_ir_base,
	  "Set IR base channel to N (1..3) [1]", "N" },
	{ "ir-source", 's', 0, G_OPTION_ARG_INT, &st_ir_source,
	  "Set tuner source to N (0:Terestrial 1:BS 2:CS)", "N" },
	{ "ir-channel", 'c', 0, G_OPTION_ARG_INT, &st_ir_channel,
	  "Set tuner channel to C (1..12)", "C" },
	{ "ir-three-channel", '3', 0, G_OPTION_ARG_STRING, &st_ir_three_channel,
	  "Set tuner channel to CCC (BS/CS only)", "CCC" },
	{ NULL }
};

static gboolean st_b25_is_enabled = FALSE;
static gint st_b25_round = 4;
static gboolean st_b25_strip = FALSE;
static gint st_b25_bcas_queue_size = 256;
static GOptionEntry st_b25_options[] = {
	{ "b25-enable", 'B', 0, G_OPTION_ARG_NONE, &st_b25_is_enabled,
	  "Enable B25 decoder [disabled]", NULL },
	{ "b25-round", 0, 0, G_OPTION_ARG_INT, &st_b25_round,
	  "Set MULTI-2 round factor to N [4]", "N" },
	{ "b25-strip", 'S', 0, G_OPTION_ARG_NONE, &st_b25_strip,
	  "Discard NULL packets from output [disabled]", NULL },
	{ "b25-bcas-queue-size", 0, 0, G_OPTION_ARG_INT, &st_b25_bcas_queue_size,
	  "Set ECM buffer capacity of pseudo B-CAS reader to N [256]", "N" },
	{ NULL }
};

static gchar *st_ts_filename = NULL;
static gchar *st_bcas_filename = NULL;
static gboolean st_is_verbose = FALSE;
static gboolean st_is_quiet = FALSE;
static GOptionEntry st_main_options[] = {
	{ "ts-filename", 't', 0, G_OPTION_ARG_FILENAME, &st_ts_filename,
	  "Output MPEG2-TS to FILENAME", "FILENAME" },
	{ "bcas-filename", 'b', 0, G_OPTION_ARG_FILENAME, &st_bcas_filename,
	  "Output B-CAS stream to FILENAME", "FILENAME" },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &st_is_verbose,
	  "Verbose messages [disabled]", NULL },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &st_is_quiet,
	  "Quiet messages [disabled]", NULL },
	{ NULL }
};


static gboolean st_is_running = TRUE;
static ARIB_STD_B25 *st_b25 = NULL;
static B_CAS_CARD *st_bcas = NULL;


/* Ring buffer
   -------------------------------------------------------------------------- */
static GQueue *st_b25_queue = NULL;
static gsize st_b25_queue_size = 0;
static gsize st_b25_queue_max = 64*1024*1024;

/* Callbacks
   -------------------------------------------------------------------------- */
static gboolean
transfer_ts_cb(gpointer data, gint length, gpointer user_data)
{
	GIOChannel *io = (GIOChannel *)user_data;
	GError *error = NULL;
	gsize written;

	if (st_b25) {
		ARIB_STD_B25_BUFFER buffer;
		gint ret;
		gpointer *chunk;

		/* キューに積む */
		chunk = g_slice_alloc(sizeof(gsize) + length);
		*(gsize *)chunk = length;
		memcpy(((gsize *)chunk) + 1, data, length);
		g_queue_push_tail(st_b25_queue, chunk);
		st_b25_queue_size += length;

		while (st_b25_queue_size > st_b25_queue_max) {
			gpointer pop = g_queue_pop_head(st_b25_queue);
			if (!pop) break;

			buffer.size = *(gsize *)pop;
			buffer.data = ((gsize *)pop) + 1;
			ret = st_b25->put(st_b25, &buffer);
			if (ret < 0) {
				g_critical("b25->put %d", ret);
			}

			g_slice_free1(sizeof(gsize) + buffer.size, pop);
			st_b25_queue_size -= buffer.size;

			ret = st_b25->get(st_b25, &buffer);
			if (ret < 0) {
				g_critical("b25->get %d", ret);
			}
			if (io) {
				g_io_channel_write_chars(io, buffer.data, buffer.size, &written, &error);
			}
		}
	} else if (io) {
		g_io_channel_write_chars(io, data, length, &written, &error);
	}
	return TRUE;
}

static gboolean
transfer_bcas_cb(gpointer data, gint length, gpointer user_data)
{
	GIOChannel *io = (GIOChannel *)user_data;
	GError *error = NULL;
	gsize written;

	if (st_bcas) {
		g_debug("bcas_card_streaming_push: len=%d", length);
		bcas_card_streaming_push(st_bcas, data, length);
	}
	if (io) {
		g_io_channel_write_chars(io, data, length, &written, &error);
	}
	return TRUE;
}


static gboolean
init_b25(void)
{
	g_message("Initializing B25 decoder");
	st_b25 = create_arib_std_b25();
	if (!st_b25) {
		g_critical("couldn't create B25 decoder");
		return FALSE;
	}

	g_message("Set MULTI-2 round factor to %d", st_b25_round);
	st_b25->set_multi2_round(st_b25, st_b25_round);

	g_message("%s omit NULL packets", st_b25_strip ? "Enable" : "Disable");
	st_b25->set_strip(st_b25, 0);

	g_message("Initializing B-CAS card reader emulator");
	st_bcas = bcas_card_streaming_new();
	if (!st_bcas) {
		g_critical("couldn't create B-CAS card reader");
		return FALSE;
	}
	if (st_bcas->init(st_bcas)) {
		g_critical("couldn't initialize B-CAS card reader");
		return FALSE;
	}

	g_message("Set B-CAS ECM buffer queue length to %d", st_b25_bcas_queue_size);
	bcas_card_streaming_set_queue_len(st_bcas, st_b25_bcas_queue_size);

	st_b25->set_b_cas_card(st_b25, st_bcas);

	st_b25_queue = g_queue_new();

	return TRUE;
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

	if (st_b25_is_enabled) {
		if (!init_b25()) {
			return;
		}
	}

	g_message("capsts_set_ir_base: %d", st_ir_base);
	capsts_set_ir_base(st_ir_base);

	device = capsts_open(st_fx2_id, st_fx2_is_force_load);
	if (!device) {
		return;
	}

	capsts_cmd_push(CMD_PORT_CFG, 0x00, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
	capsts_cmd_push(CMD_MODE_IDLE);
	capsts_cmd_push(CMD_IFCONFIG, 0xE3);
	capsts_cmd_commit(device);

	capsts_adjust_tuner_channel(device, st_ir_source, st_ir_channel, st_ir_three_channel);

	if (st_ts_filename) {
		io_ts = g_io_channel_new_file(st_ts_filename, "w", &error);
		if (!io_ts) {
			g_critical("Couldn't open TS output file %s", st_ts_filename);
			goto quit;
		}
		g_io_channel_set_encoding(io_ts, NULL, &error);

		g_message("Setup TS transfer");
		transfer_ts = cusbfx2_init_bulk_transfer(device, "MPEG-TS", ENDPOINT_TS_IN,
												 st_fx2_ts_buffer_size, st_fx2_ts_buffer_count,
												 transfer_ts_cb, io_ts);
		if (!transfer_ts) {
			g_critical("Couldn't setup TS transfer");
			goto quit;
		}
		capsts_cmd_push(CMD_EP6IN_START);
	}

	if (st_bcas_filename) {
		io_bcas = g_io_channel_new_file(st_bcas_filename, "w", &error);
		if (!io_bcas) {
			g_critical("Couldn't open B-CAS output file %s", st_bcas_filename);
			goto quit;
		}
		g_io_channel_set_encoding(io_bcas, NULL, &error);
	}

	if (st_bcas_filename || st_b25_is_enabled) {
		g_message("Setup B-CAS transfer");
		transfer_bcas = cusbfx2_init_bulk_transfer(device, "B-CAS", ENDPOINT_BCAS_IN,
												   128, 1, transfer_bcas_cb, io_bcas);
		if (!transfer_bcas) {
			g_critical("Couldn't setup B-CAS transfer");
			goto quit;
		}
		capsts_cmd_push(CMD_EP4IN_START);
	}

	capsts_cmd_push(CMD_PORT_WRITE, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
	capsts_cmd_commit(device);

	timer = g_timer_new();
	while (st_is_running) {
		gdouble elapsed;
		cusbfx2_poll();
		elapsed = g_timer_elapsed(timer, NULL);
		g_fprintf(stderr, "> Now:%.1f TS:%d(%3d%%)\r",
				  elapsed,
				  st_b25_queue_size,
				  (gsize)((gdouble)st_b25_queue_size / st_b25_queue_max * 100));
		if (elapsed > 60.0) {
			break;
		}
	}
	g_fprintf(stderr, "\n");
	g_timer_destroy(timer);

	if (st_is_running) {
		g_message("### FINISHED SHUTDOWN ###");
	} else {
		g_message("### INTERRUPTED SHUTDOWN ###");
	}

	if (st_ts_filename) capsts_cmd_push(CMD_EP6IN_STOP);
	if (st_bcas_filename || st_b25_is_enabled) capsts_cmd_push(CMD_EP4IN_STOP);
	capsts_cmd_push(CMD_MODE_IDLE);
	capsts_cmd_commit(device);

 quit:
	cusbfx2_free_transfer(transfer_ts);
	cusbfx2_free_transfer(transfer_bcas);
	if (io_ts) g_io_channel_close(io_ts);
	if (io_bcas) g_io_channel_close(io_bcas);
	cusbfx2_close(device);
}

static gboolean
parse_options(int *argc, char ***argv)
{
	GError *error = NULL;
	GOptionContext *context;
	GOptionGroup *group_fx2, *group_ir, *group_b25;

	context = g_option_context_new("-- MPEG2-TS Sniffer");
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, st_main_options, NULL);

	group_fx2 = g_option_group_new("fx2", "Chameleon USB FX2 Options:", "Show CUSBFX2 options", NULL, NULL);
	g_option_group_add_entries(group_fx2, st_fx2_options);
	g_option_context_add_group(context, group_fx2);

	group_ir = g_option_group_new("ir", "Infrared Receiver Control Options:", "Show IR options", NULL, NULL);
	g_option_group_add_entries(group_ir, st_ir_options);
	g_option_context_add_group(context, group_ir);

	group_b25 = g_option_group_new("b25", "ARIB STD-B25 Deocder Options:", "Show ARIB STD-B25 deocder options", NULL, NULL);
	g_option_group_add_entries(group_b25, st_b25_options);
	g_option_context_add_group(context, group_b25);

	if (!g_option_context_parse(context, argc, argv, &error)) {
		return FALSE;
	}

	st_ir_base = CLAMP(st_ir_base, 1, 3);
	st_ir_source = CLAMP(st_ir_source, -1, 2);
	st_ir_channel = CLAMP(st_ir_channel, -1, 12);

	/* 3桁チャンネルのフォーマットチェック */
	if (st_ir_three_channel) {
		gboolean is_valid = TRUE;
		gchar *p;
		if (strlen(st_ir_three_channel) != 3) {
			is_valid = FALSE;
		} else {
			for (p = st_ir_three_channel; *p; ++p) {
				if (!g_ascii_isdigit(*p)) {
					is_valid = FALSE;
					break;
				}
			}
		}
		if (!is_valid) {
			g_critical("Invalid channel format");
			return FALSE;
		}
	}

	return TRUE;
}

static void
sighandler(int signum)
{
	st_is_running = FALSE;
}

static void
log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
	gchar asctime[32];
	GTimeVal now;
	const gchar *level;
	struct tm now_tm;

	switch (log_level) {
	case G_LOG_LEVEL_ERROR:
		level = "ERROR";
		break;
	case G_LOG_LEVEL_CRITICAL:
		level = "FATAL";
		break;
	case G_LOG_LEVEL_WARNING:
		level = "WARN ";
		break;
	case G_LOG_LEVEL_MESSAGE:
	case G_LOG_LEVEL_INFO:
		if (st_is_quiet) return;
		level = "INFO ";
		break;
	case G_LOG_LEVEL_DEBUG:
		if (!st_is_verbose) return;
		level = "DEBUG";
		break;
	default:
		level = "?????";
	}

	g_get_current_time(&now);
	localtime_r(&now.tv_sec, &now_tm);
	
	strftime(asctime, sizeof(asctime), "%Y-%m-%d %H:%M:%S", &now_tm);

	g_fprintf(stderr, "%s,%03d %s %s\n", asctime, now.tv_usec / 1000, level, message);
}

static void
log_handler_null(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
}

int
main(int argc, char **argv)
{
	GLogLevelFlags log_level = G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION;
	struct sigaction sigact;

	if (!parse_options(&argc, &argv)) {
		return 1;
	}

	g_log_set_handler(NULL, log_level, log_handler, NULL);
	g_log_set_handler("cusbfx2", log_level, log_handler, NULL);
	g_log_set_handler("capsts", log_level, log_handler, NULL);
	g_log_set_handler("GLib", log_level, log_handler, NULL);

	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);

	cusbfx2_init();

	rec();

	cusbfx2_exit();

	return 0;
}
