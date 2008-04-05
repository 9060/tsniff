#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <glib.h>
#include "cusbfx2.h"
#include "capsts.h"
#include "arib_std_b25.h"
#include "b_cas_card.h"
#include "pseudo_bcas.h"
#include "bcas_stream.h"

#define TS_INPUT_FX2_PREFIX "fx2:"
#define BCAS_INPUT_FX2_PREFIX "fx2:"
#define BCAS_INPUT_PCSC_PREFIX "pcsc:"

/* Options
   -------------------------------------------------------------------------- */
//#define OPTION_FLAG_DEBUG G_OPTION_FLAG_HIDDEN
#define OPTION_FLAG_DEBUG 0

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
static gchar *st_b25_system_key = NULL;
static gchar *st_b25_init_cbc = NULL;
static GOptionEntry st_b25_options[] = {
	{ "b25-round", 0, 0, G_OPTION_ARG_INT, &st_b25_round,
	  "Set MULTI-2 round factor to N [4]", "N" },
	{ "b25-strip", 'S', 0, G_OPTION_ARG_NONE, &st_b25_strip,
	  "Discard NULL packets from output [disabled]", NULL },
	{ "b25-bcas-queue-size", 0, 0, G_OPTION_ARG_INT, &st_b25_bcas_queue_size,
	  "Set ECM buffer capacity of pseudo B-CAS reader to N [256]", "N" },
	{ "b25-system-key", 0, 0, G_OPTION_ARG_STRING, &st_b25_system_key,
	  "Set B25 system key to HEX when using pseudo B-CAS reader", "HEX" },
	{ "b25-init-cbc", 0, 0, G_OPTION_ARG_STRING, &st_b25_init_cbc,
	  "Set B25 Init-CBC to HEX when using pseudo B-CAS reader", "HEX" },
	{ NULL }
};

static gchar *st_ts_input = BCAS_INPUT_FX2_PREFIX;
static gchar *st_bcas_input = BCAS_INPUT_FX2_PREFIX;
static gchar *st_ts_output = NULL;
static gchar *st_bcas_output = NULL;
static gchar *st_b25_output = NULL;
static gboolean st_is_verbose = FALSE;
static gboolean st_is_quiet = FALSE;
static gint st_verify_bcas_stream = -1;
static GOptionEntry st_main_options[] = {
	{ "ts-input", 'T', 0, G_OPTION_ARG_FILENAME, &st_ts_input,
	  "Input MPEG2-TS from SOURCE (" BCAS_INPUT_FX2_PREFIX " or FILENAME) [" BCAS_INPUT_FX2_PREFIX "]", "SOURCE" },
	{ "bcas-input", 'B', 0, G_OPTION_ARG_FILENAME, &st_bcas_input,
	  "Input B-CAS from SOURCE (" BCAS_INPUT_FX2_PREFIX " or " BCAS_INPUT_PCSC_PREFIX " or FILENAME) [" BCAS_INPUT_FX2_PREFIX ":]", "SOURCE" },

	{ "ts-output", 't', 0, G_OPTION_ARG_FILENAME, &st_ts_output,
	  "Output raw MPEG2-TS to FILENAME", "FILENAME" },
	{ "bcas-output", 'b', 0, G_OPTION_ARG_FILENAME, &st_bcas_output,
	  "Output B-CAS to FILENAME", "FILENAME" },
	{ "b25-output", 'o', 0, G_OPTION_ARG_FILENAME, &st_b25_output,
	  "Enable ARIB STD-B25 decoder and output to FILENAME", "FILENAME" },

	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &st_is_verbose,
	  "Verbose messages [disabled]", NULL },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &st_is_quiet,
	  "Quiet messages [disabled]", NULL },

	{ "verify-bcas-stream", 0, OPTION_FLAG_DEBUG, G_OPTION_ARG_INT, &st_verify_bcas_stream,
	  "Verify B-CAS stream CHUNK_SIZE=N (1-512)", "N" },
	{ NULL }
};


static gboolean st_is_running = TRUE;
static gboolean st_is_use_cusbfx2 = FALSE;
static ARIB_STD_B25 *st_b25 = NULL;
static B_CAS_CARD *st_bcas = NULL;
static GIOChannel *st_ts_input_io = NULL;
static GIOChannel *st_bcas_input_io = NULL;
static GIOChannel *st_ts_output_io = NULL;
static GIOChannel *st_bcas_output_io = NULL;
static GIOChannel *st_b25_output_io = NULL;


/* Ring buffer
   -------------------------------------------------------------------------- */
static GQueue *st_b25_queue = NULL;
static gsize st_b25_queue_size = 0;
static gsize st_b25_queue_max = 32*1024*1024;


/* Signal handler
   -------------------------------------------------------------------------- */
static guint st_installed_sighandler = 0;
struct sigaction st_old_sigaction_sigint;
struct sigaction st_old_sigaction_sigterm;
struct sigaction st_old_sigaction_sigquit;

static void
sighandler(int signum)
{
	st_is_running = FALSE;
}

static void
install_sighandler(void)
{
	struct sigaction sigact;

	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;

	if (sigaction(SIGINT, &sigact, &st_old_sigaction_sigint) == 0) st_installed_sighandler |= 1;
	if (sigaction(SIGTERM, &sigact, &st_old_sigaction_sigterm) == 0) st_installed_sighandler |= 2;
	if (sigaction(SIGQUIT, &sigact, &st_old_sigaction_sigquit) == 0) st_installed_sighandler |= 4;
}

static void
restore_sighandler(void)
{
	if (st_installed_sighandler & 1) sigaction(SIGINT, &st_old_sigaction_sigint, NULL);
	if (st_installed_sighandler & 2) sigaction(SIGTERM, &st_old_sigaction_sigterm, NULL);
	if (st_installed_sighandler & 4) sigaction(SIGQUIT, &st_old_sigaction_sigquit, NULL);
}


/* Callbacks
   -------------------------------------------------------------------------- */
static void
proc_b25(gpointer data, gsize length)
{
	ARIB_STD_B25_BUFFER buffer;
	gint r;

	buffer.size = length;
	buffer.data = data;
	r = st_b25->put(st_b25, &buffer);
	if (r < 0) {
		g_warning("!!! ARIB_STD_B25::put failed (%d)", r);
	}

	r = st_b25->get(st_b25, &buffer);
	if (r < 0) {
		g_warning("!!! ARIB_STD_B25::get failed (%d)", r);
	} else if (buffer.size > 0) {
		GError *error = NULL;
		gsize written;
		g_io_channel_write_chars(st_b25_output_io, buffer.data, buffer.size, &written, &error);
		if (error) {
			g_warning("[transfer_ts_callback] %s", error->message);
			g_clear_error(&error);
		}
	}
}

static gboolean
transfer_ts_cb(gpointer data, gint length, gpointer user_data)
{
	GError *error = NULL;
	gsize written;

	if (st_ts_output_io) {
		g_io_channel_write_chars(st_ts_output_io, data, length, &written, &error);
		if (error) {
			g_warning("[transfer_ts_callback] %s", error->message);
			g_clear_error(&error);
		}
	}

	if (st_b25_output_io) {
		gint ret;
		gpointer *chunk;

		if (st_b25_queue) {
			/* キューに積む */
			chunk = g_slice_alloc(sizeof(gsize) + length);
			*(gsize *)chunk = length;
			memcpy(((gsize *)chunk) + 1, data, length);
			g_queue_push_tail(st_b25_queue, chunk);
			st_b25_queue_size += length;

			while (st_b25_queue_size > st_b25_queue_max) {
				gpointer pop = g_queue_pop_head(st_b25_queue);
				gsize size = *(gsize *)pop;

				if (!pop) break;

				proc_b25((gpointer)(((gsize *)pop) + 1), size);

				g_slice_free1(sizeof(gsize) + size, pop);
				st_b25_queue_size -= size;
			}
		} else {
			proc_b25(data, length);
		}
	}

	return st_is_running;
}

static gboolean
transfer_bcas_cb(gpointer data, gint length, gpointer user_data)
{
	if (st_bcas) {
		((PSEUDO_B_CAS_CARD *)st_bcas)->push(st_bcas, data, length);
	}

	if (st_bcas_output_io) {
		gsize written;
		GError *error = NULL;
		g_io_channel_write_chars(st_bcas_output_io, data, length, &written, &error);
		if (error) {
			g_warning("[transfer_bcas_callback] %s", error->message);
			g_clear_error(&error);
		}
	}

	return st_is_running;
}


static GIOChannel *
open_io_channel(const gchar *filename, gboolean is_output)
{
	GIOChannel *io;
	GError *error = NULL;

	if (!strcmp(filename, "-")) {
		io = g_io_channel_unix_new(is_output ? 1 : 0);
	} else {
		io = g_io_channel_new_file(filename, is_output ? "w" : "r", &error);
	}

	if (error) {
		g_critical("[open_io_channel] %s", error->message);
		g_clear_error(&error);
		return NULL;
	}

	g_io_channel_set_encoding(io, NULL, &error);
	if (error) {
		g_critical("[open_io_channel] %s", error->message);
		g_clear_error(&error);
		g_io_channel_shutdown(io, FALSE, NULL);
		return NULL;
	}

	return io;
}

static gboolean
init_b25(void)
{
	gboolean is_pseudo_bcas = TRUE;
	gint r;

	/* B-CAS カードクラスの初期化 */
	if (g_str_has_prefix(st_bcas_input, BCAS_INPUT_PCSC_PREFIX)) {
		g_message("*** using real B-CAS card reader");
		st_bcas = create_b_cas_card();
		is_pseudo_bcas = FALSE;
	} else {
		st_bcas = (B_CAS_CARD *)pseudo_bcas_new();

		if (g_str_has_prefix(st_bcas_input, BCAS_INPUT_FX2_PREFIX)) {
			g_message("*** using pseudo B-CAS card reader with CUSBFX2");
		} else {
			g_message("*** using pseudo B-CAS card reader with <%s>", st_bcas_input);
		}
	}
	if (!st_bcas) {
		g_critical("!!! couldn't create B-CAS card reader");
		return FALSE;
	}

	r = st_bcas->init(st_bcas);
	if (r < 0) {
		g_critical("!!! couldn't initialize B-CAS card reader (%d)", r);
		return FALSE;
	}

	/* 仮想 B-CAS カードを使う場合の追加初期化 */
	if (is_pseudo_bcas) {
		if (!((PSEUDO_B_CAS_CARD *)st_bcas)->set_init_status_from_hex(st_bcas, st_b25_system_key, st_b25_init_cbc)) {
			g_critical("!!! B-CAS SYSTEM KEY AND INIT-CBC NOT SUPPLIED");
			st_bcas->release(st_bcas);
			return FALSE;
		}

		if (g_str_has_prefix(st_bcas_input, BCAS_INPUT_FX2_PREFIX)) {
			g_message("*** set B-CAS ECM buffer queue length to %d", st_b25_bcas_queue_size);
			((PSEUDO_B_CAS_CARD *)st_bcas)->set_queue_len(st_bcas, st_b25_bcas_queue_size);
		} else {
			((PSEUDO_B_CAS_CARD *)st_bcas)->set_queue_len(st_bcas, G_MAXUINT);
		}
	}

	g_message("*** initializing B25 decoder");
	st_b25 = create_arib_std_b25();
	if (!st_b25) {
		g_critical("!!! couldn't create B25 decoder");
		st_bcas->release(st_bcas);
		return FALSE;
	}

	g_message("*** set MULTI-2 round factor to %d", st_b25_round);
	st_b25->set_multi2_round(st_b25, st_b25_round);

	g_message("*** %s omit NULL packets", st_b25_strip ? "Enable" : "Disable");
	st_b25->set_strip(st_b25, st_b25_strip ? 1 : 0);

	st_b25->set_b_cas_card(st_b25, st_bcas);

	/* Initialize TS time-shift buffer */
	if (g_str_has_prefix(st_bcas_input, BCAS_INPUT_FX2_PREFIX)) {
		st_b25_queue = g_queue_new();
		g_message("*** Initalize TS time-shift buffer");
	}

	return TRUE;
}

static void
release_b25(void)
{
	if (st_b25_queue) {
		/* TODO: free chunks */
		g_queue_free(st_b25_queue);
	}
	if (st_b25) st_b25->release(st_b25);
	if (st_bcas) st_bcas->release(st_bcas);
}

static void
info_b25(ARIB_STD_B25 *b25)
{
	ARIB_STD_B25_PROGRAM_INFO info;
	gint n_programs, i;

	n_programs = b25->get_program_count(b25);
	if (n_programs < 0) {
		g_warning("!!! ARIB_STD_B25::get_program_count failed (%d)", n_programs);
	} else {
		for (i = 0; i < n_programs; ++i) {
			gint r;
			r = b25->get_program_info(b25, &info, i);
			if (r < 0) {
				g_warning("!!! ARIB_STD_B25::get_program_info(%d) failed (%d)", i, r);
			} else {
				g_message("> CH:%5d unpurchased ECM:%d(%04x)"
						  " undecrypted TS:%8"G_GINT64_FORMAT"/%8"G_GINT64_FORMAT" (%.3f%%)",
						  info.program_number,
						  info.ecm_unpurchased_count,
						  info.last_ecm_error_code,
						  info.undecrypted_packet_count,
						  info.total_packet_count,
						  (gdouble)info.undecrypted_packet_count / info.total_packet_count * 100);
			}
		}
	}
}

static void
run(void)
{
	cusbfx2_handle *device = NULL;
	cusbfx2_transfer *transfer_ts = NULL;
	cusbfx2_transfer *transfer_bcas = NULL;
	GError *error = NULL;
	GTimer *timer; 
	gboolean is_cusbfx2_inited = FALSE;
	gboolean is_cusbfx2_started = FALSE;

	/* Initialize inputs */
	if (!g_str_has_prefix(st_ts_input, TS_INPUT_FX2_PREFIX)) {
		if (!(st_ts_input_io = open_io_channel(st_ts_input, FALSE))) {
			g_critical("!!! couldn't open TS input <%s>", st_ts_input);
			goto quit;
		}
	}

	if (!g_str_has_prefix(st_bcas_input, BCAS_INPUT_FX2_PREFIX) && !g_str_has_prefix(st_bcas_input, BCAS_INPUT_PCSC_PREFIX)) {
		if (!(st_bcas_input_io = open_io_channel(st_bcas_input, FALSE))) {
			g_critical("!!! couldn't open B-CAS input <%s>", st_bcas_input);
			goto quit;
		}
	}

	/* Initialize outputs */
	if (st_ts_output) {
		if (!(st_ts_output_io = open_io_channel(st_ts_output, TRUE))) {
			g_critical("!!! couldn't open TS output <%s>", st_ts_output);
			goto quit;
		}
	}
	if (st_bcas_output) {
		if (!(st_bcas_output_io = open_io_channel(st_bcas_output, TRUE))) {
			g_critical("!!! couldn't open TS output <%s>", st_bcas_output);
			goto quit;
		}
	}
	if (st_b25_output) {
		if (!(st_b25_output_io = open_io_channel(st_b25_output, TRUE))) {
			g_critical("!!! couldn't open B25 output <%s>", st_b25_output);
			goto quit;
		}
	}

	/* Initialize B25 */
	if (st_b25_output) {
		if (!init_b25()) {
			goto quit;
		}
	}

	/* Initialize CUSBFX2 */
	if (st_is_use_cusbfx2) {
		cusbfx2_init();
		is_cusbfx2_inited = TRUE;

		device = capsts_open(st_fx2_id, st_fx2_is_force_load);
		if (!device) {
			goto quit;
		}

		capsts_cmd_push(CMD_PORT_CFG, 0x00, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
		capsts_cmd_push(CMD_MODE_IDLE);
		capsts_cmd_push(CMD_IFCONFIG, 0xE3);
		capsts_cmd_commit(device);

		capsts_set_ir_base(st_ir_base);
		capsts_adjust_tuner_channel(device, st_ir_source, st_ir_channel, st_ir_three_channel);

		if (g_str_has_prefix(st_ts_input, TS_INPUT_FX2_PREFIX)) {
			g_message("*** setup TS transfer");
			transfer_ts = cusbfx2_init_bulk_transfer(device, "TS", ENDPOINT_TS_IN,
													 st_fx2_ts_buffer_size, st_fx2_ts_buffer_count,
													 transfer_ts_cb, NULL);
			if (!transfer_ts) {
				g_critical("!!! couldn't setup TS transfer");
				goto quit;
			}
			capsts_cmd_push(CMD_EP6IN_START);
		}

		if (g_str_has_prefix(st_bcas_input, BCAS_INPUT_FX2_PREFIX)) {
			g_message("*** setup B-CAS transfer");
			transfer_bcas = cusbfx2_init_bulk_transfer(device, "B-CAS", ENDPOINT_BCAS_IN,
													   32, 1, transfer_bcas_cb, NULL);
			if (!transfer_bcas) {
				g_critical("!!! couldn't setup B-CAS transfer");
				goto quit;
			}
			capsts_cmd_push(CMD_EP4IN_START);
		}

		capsts_cmd_push(CMD_PORT_WRITE, PIO_START|PIO_IR_OUT|PIO_TS_BACK);
		capsts_cmd_commit(device);
		is_cusbfx2_started = TRUE;
	}

	install_sighandler();

	/* B-CAS 入力がファイルであれば、事前に読んでおく */
	if (st_bcas_input_io) {
		for (;;) {
			GError *error = NULL;
			guint8 buf[512];
			gsize readed;

			if (g_io_channel_read_chars(st_bcas_input_io, buf, 512, &readed, &error) != G_IO_STATUS_NORMAL) {
				break;
			}
			if (error) {
				g_warning("!!! %s", error->message);
				g_clear_error(&error);
			}

			if (st_bcas_output_io) {
				gsize written;
				g_io_channel_write_chars(st_bcas_output_io, buf, readed, &written, &error);
				if (error) {
					g_warning("!!! %s", error->message);
					g_clear_error(&error);
				}
			}

			if (st_bcas)
				((PSEUDO_B_CAS_CARD *)st_bcas)->push(st_bcas, buf, readed);
		}
	}

	/* main loop */
	timer = g_timer_new();
	while (st_is_running) {
		gdouble elapsed;

		if (st_ts_input_io) {
			GError *error = NULL;
			guint8 buf[512];
			gsize readed;
			GIOStatus status;

			status = g_io_channel_read_chars(st_ts_input_io, buf, 512, &readed, &error);
			if (status == G_IO_STATUS_NORMAL) {
				transfer_ts_cb(buf, 512, NULL);
			} else {
				if (error) {
					g_warning("!!! %s", error->message);
					g_clear_error(&error);
				}
				break;
			}
		} else if (is_cusbfx2_started) {
			cusbfx2_poll();

			elapsed = g_timer_elapsed(timer, NULL);
			g_fprintf(stderr, ">>> Now:%.1f TS:%d(%3d%%)\r",
					  elapsed,
					  st_b25_queue_size,
					  (gsize)((gdouble)st_b25_queue_size / st_b25_queue_max * 100));
			if (elapsed > 180.0) {
				break;
			}
		}
	}
	g_fprintf(stderr, "\n");
	g_timer_destroy(timer);

	if (st_is_running) {
		g_message("### FINISHED SHUTDOWN ###");
	} else {
		g_message("### INTERRUPTED SHUTDOWN ###");
	}

 quit:
	/* finalize */
	restore_sighandler();

	if (st_is_use_cusbfx2) {
		if (is_cusbfx2_started) {
			if (transfer_ts) capsts_cmd_push(CMD_EP6IN_STOP);
			if (transfer_bcas) capsts_cmd_push(CMD_EP4IN_STOP);
			capsts_cmd_push(CMD_MODE_IDLE);
			capsts_cmd_commit(device);
		}

		/* TODO: poll? */

		if (transfer_ts) cusbfx2_free_transfer(transfer_ts);
		if (transfer_bcas) cusbfx2_free_transfer(transfer_bcas);

		if (device) cusbfx2_close(device);
		if (is_cusbfx2_inited) cusbfx2_exit();
	}

	/* flush */
	if (st_b25) {
		gint r;
		ARIB_STD_B25_BUFFER buffer;

		if (st_b25_queue) {
			gpointer chunk;
			while (chunk = g_queue_pop_head(st_b25_queue)) {
				gsize size = *(gsize *)chunk;
				proc_b25((gpointer)(((gsize *)chunk) + 1), size);
				g_slice_free1(sizeof(gsize) + size, chunk);
				st_b25_queue_size -= size;
			}
		}

		r = st_b25->flush(st_b25);
		if (r < 0) {
			g_warning("!!! ARIB_STD_B25::flush failed (%d)", r);
		}

		r = st_b25->get(st_b25, &buffer);
		if (r < 0) {
			g_warning("!!! ARIB_STD_B25::get failed (%d)", r);
		} else if (buffer.size > 0) {
			GError *error = NULL;
			gsize written;
			g_io_channel_write_chars(st_b25_output_io, buffer.data, buffer.size, &written, &error);
			if (error) {
				g_warning("!!! %s", error->message);
				g_clear_error(&error);
			}
		}

		info_b25(st_b25);
	}

	release_b25();

	if (st_b25_output_io) g_io_channel_shutdown(st_b25_output_io, TRUE, NULL);
	if (st_bcas_output_io) g_io_channel_shutdown(st_bcas_output_io, TRUE, NULL);
	if (st_ts_output_io) g_io_channel_shutdown(st_ts_output_io, TRUE, NULL);
	if (st_bcas_input_io) g_io_channel_shutdown(st_bcas_input_io, TRUE, NULL);
	if (st_ts_input_io) g_io_channel_shutdown(st_ts_input_io, TRUE, NULL);
}

/**
 * --verify-bcas-stream=N と --bcas-filename=FILENAME が指定されて場合に実行される。
 *
 * B-CASストリームを --bcas-filename で指定されたファイルから読み取る。
 * ストリームは --verify-bcas-stream=N で指定されたサイズで分割されつつ、
 * パーザに送られる。
 * パーザでエラーが出なければおけ。
 */
static void
verify_bcas_stream(void)
{
#if 0
	guint8 *buf;
	gsize readed;
	GIOChannel *io_bcas = NULL;
	GError *error = NULL;
	BCASStream *bcas_stream = NULL;
	gint chunk_size = st_verify_bcas_stream;

	g_message("*** Verify B-CAS Stream ***");

	buf = g_malloc(chunk_size);
	io_bcas = g_io_channel_new_file(st_bcas_filename, "r", &error);
	if (!io_bcas) {
			return;
	}
	g_io_channel_set_encoding(io_bcas, NULL, &error);
	bcas_stream = bcas_stream_new();
	while (g_io_channel_read_chars(io_bcas, buf, chunk_size, &readed, &error) == G_IO_STATUS_NORMAL) {
		bcas_stream_push(bcas_stream, buf, readed, NULL, NULL);
	}

	g_free(buf);
	bcas_stream_free(bcas_stream);
	g_io_channel_close(io_bcas);
	
	return;
#endif
}

static void
load_key_file(void)
{
	GKeyFile *keyfile;
	GError *error = NULL;
	GString *path;

	keyfile = g_key_file_new();

	path = g_string_new(g_get_user_config_dir());
	g_string_append(path, "/tsniff.conf");
	g_message("[load_key_file] looking for %s", path->str);

	if (g_key_file_load_from_file(keyfile, path->str, G_KEY_FILE_NONE, &error)) {
		st_b25_system_key = g_key_file_get_string(keyfile, "b25", "system_key", NULL);
		st_b25_init_cbc = g_key_file_get_string(keyfile, "b25", "init_cbc", NULL);
	} else {
		if (error) g_warning("[load_key_file] %s", error->message);
		g_clear_error(&error);
	}

	g_string_free(path, TRUE);
	g_key_file_free(keyfile);
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

	group_b25 = g_option_group_new("b25", "ARIB STD-B25 Decoder Options:", "Show ARIB STD-B25 decoder options", NULL, NULL);
	g_option_group_add_entries(group_b25, st_b25_options);
	g_option_context_add_group(context, group_b25);

	if (!g_option_context_parse(context, argc, argv, &error)) {
		return FALSE;
	}

	st_ir_base = CLAMP(st_ir_base, 1, 3);
	st_ir_source = CLAMP(st_ir_source, -1, 2);
	st_ir_channel = CLAMP(st_ir_channel, -1, 12);

	if (g_str_has_prefix(st_ts_input, TS_INPUT_FX2_PREFIX) || g_str_has_prefix(st_bcas_input, TS_INPUT_FX2_PREFIX)) {
		st_is_use_cusbfx2 = TRUE;
	}

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

	g_log_set_handler(NULL, log_level, log_handler, NULL);
	g_log_set_handler("cusbfx2", log_level, log_handler, NULL);
	g_log_set_handler("capsts", log_level, log_handler, NULL);
	g_log_set_handler("GLib", log_level, log_handler, NULL);

	load_key_file();
	if (!parse_options(&argc, &argv)) {
		return 1;
	}

	if (st_verify_bcas_stream > 0) {
		verify_bcas_stream();
	} else {
		run();
	}

	return 0;
}
