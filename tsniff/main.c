#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <glib.h>
#include "config.h"
#include "cusbfx2.h"
#include "capsts.h"
#include "arib_std_b25.h"
#include "b_cas_card.h"
#include "pseudo_bcas.h"
#include "bcas_stream.h"


#define INPUT_TYPE_FX2_PREFIX "fx2:"
#define INPUT_TYPE_PCSC_PREFIX "pcsc:"
#define INPUT_TYPE_FX2 0
#define INPUT_TYPE_FILE 1
#define INPUT_TYPE_PCSC 2
static gint st_ts_input_type;
static gint st_bcas_input_type;

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
	{ "fx2-ts-buffer-count", 0, 0, G_OPTION_ARG_INT, &st_fx2_ts_buffer_count,
	  "Set TS transfer buffer count to N [16]", "N" },
	{ NULL }
};

static gint st_ir_base = 0;
static gint st_ir_source = -1;
static gchar *st_ir_channel = NULL;
static GOptionEntry st_ir_options[] = {
	{ "ir-base", 0, 0, G_OPTION_ARG_INT, &st_ir_base,
	  "Set IR base channel to N (1..3) [1]", "N" },
	{ "ir-source", 's', 0, G_OPTION_ARG_INT, &st_ir_source,
	  "Set tuner source to N (0:Terestrial 1:BS 2:CS)", "N" },
	{ "ir-channel", 'c', 0, G_OPTION_ARG_STRING, &st_ir_channel,
	  "Set tuner channel to C (1..12 or 000...999)", "C" },
	{ NULL }
};

static gint st_b25_round = 4;
static gboolean st_b25_strip = FALSE;
static gdouble st_b25_ts_delay = 0.5;
static gchar *st_b25_ts_delay_string = NULL;
static gint st_b25_bcas_queue_size = 256;
static gchar *st_b25_system_key = NULL;
static gchar *st_b25_init_cbc = NULL;
static GOptionEntry st_b25_options[] = {
	{ "b25-round", 0, 0, G_OPTION_ARG_INT, &st_b25_round,
	  "Set MULTI-2 round factor to N [4]", "N" },
	{ "b25-strip", 'S', 0, G_OPTION_ARG_NONE, &st_b25_strip,
	  "Discard NULL packets from output [disabled]", NULL },
	{ "b25-ts-delay", 0, 0, G_OPTION_ARG_STRING, &st_b25_ts_delay_string,
	  "Delay TS input by N seconds, if --bcas-input="INPUT_TYPE_FX2_PREFIX" [0.5]", "N" },
	{ "b25-bcas-queue-size", 0, 0, G_OPTION_ARG_INT, &st_b25_bcas_queue_size,
	  "Set ECM buffer capacity of pseudo B-CAS reader to N [256]", "N" },
	{ "b25-system-key", 0, 0, G_OPTION_ARG_STRING, &st_b25_system_key,
	  "Set B25 system key to HEX when using pseudo B-CAS reader", "HEX" },
	{ "b25-init-cbc", 0, 0, G_OPTION_ARG_STRING, &st_b25_init_cbc,
	  "Set B25 Init-CBC to HEX when using pseudo B-CAS reader", "HEX" },
	{ NULL }
};

static gchar *st_ts_input = INPUT_TYPE_FX2_PREFIX;
static gchar *st_bcas_input = INPUT_TYPE_FX2_PREFIX;
static gchar *st_ts_output = NULL;
static gchar *st_bcas_output = NULL;
static gchar *st_b25_output = NULL;
static gint st_length = -1;
static gboolean st_is_verbose = FALSE;
static gboolean st_is_quiet = FALSE;
static gboolean st_dump_bcas_init_status = FALSE;
static gint st_verify_bcas_stream = -1;
static GOptionEntry st_main_options[] = {
	{ "ts-input", 'T', 0, G_OPTION_ARG_FILENAME, &st_ts_input,
	  "Input MPEG2-TS from SOURCE ("INPUT_TYPE_FX2_PREFIX" or FILENAME) ["INPUT_TYPE_FX2_PREFIX"]", "SOURCE" },
	{ "bcas-input", 'B', 0, G_OPTION_ARG_FILENAME, &st_bcas_input,
	  "Input B-CAS from SOURCE ("INPUT_TYPE_FX2_PREFIX" or "INPUT_TYPE_PCSC_PREFIX" or FILENAME) ["INPUT_TYPE_FX2_PREFIX"]", "SOURCE" },

	{ "ts-output", 't', 0, G_OPTION_ARG_FILENAME, &st_ts_output,
	  "Output raw MPEG2-TS to FILENAME", "FILENAME" },
	{ "bcas-output", 'b', 0, G_OPTION_ARG_FILENAME, &st_bcas_output,
	  "Output B-CAS to FILENAME", "FILENAME" },
	{ "b25-output", 'o', 0, G_OPTION_ARG_FILENAME, &st_b25_output,
	  "Enable ARIB STD-B25 decoder and output to FILENAME", "FILENAME" },

	{ "length", 'l', 0, G_OPTION_ARG_INT, &st_length,
	  "Stop sniffing when N seconds passed, if input was CUSBFX2 [infinite]", "N" },

	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &st_is_verbose,
	  "Verbose messages [disabled]", NULL },
	{ "quiet", 'q', 0, G_OPTION_ARG_NONE, &st_is_quiet,
	  "Quiet messages [disabled]", NULL },

	{ "dump-bcas-init-status", 0, 0, G_OPTION_ARG_NONE, &st_dump_bcas_init_status,
	  "Dump B-CAS init-status to STDOUT", NULL },
	{ "verify-bcas-stream", 0, OPTION_FLAG_DEBUG, G_OPTION_ARG_INT, &st_verify_bcas_stream,
	  "Verify B-CAS stream CHUNK_SIZE=N (1-512)", "N" },
	{ NULL }
};


static gboolean st_is_intterupted = FALSE;
static gboolean st_is_use_cusbfx2 = FALSE;
static ARIB_STD_B25 *st_b25 = NULL;
static B_CAS_CARD *st_bcas = NULL;
static GIOChannel *st_ts_input_io = NULL;
static GIOChannel *st_bcas_input_io = NULL;
static GIOChannel *st_ts_output_io = NULL;
static GIOChannel *st_bcas_output_io = NULL;
static GIOChannel *st_b25_output_io = NULL;

/* TS Time-shift buffer
   -------------------------------------------------------------------------- */
typedef struct {
	GTimeVal arrived_time;
	gsize size;
	/* guint8 data[size]; */
} B25Chunk;

static GQueue *st_b25_queue = NULL;
static gsize st_b25_queue_size = 0;
#define MAX_B25_QUEUE_SIZE (128*1024*1024)

/* Signal handler
   -------------------------------------------------------------------------- */
static guint st_installed_sighandler = 0;
struct sigaction st_old_sigaction_sigint;
struct sigaction st_old_sigaction_sigterm;
struct sigaction st_old_sigaction_sigquit;

static void
sighandler(int signum)
{
	st_is_intterupted = TRUE;
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


/* Threads
   -------------------------------------------------------------------------- */
static GThread *st_b25_thread = NULL;
static gboolean st_is_b25_running = TRUE;
static GAsyncQueue *st_b25_async_queue = NULL;

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
		g_io_channel_write_chars(st_b25_output_io, (gchar *)buffer.data, buffer.size, &written, &error);
		if (error) {
			g_warning("[proc_b25] %s", error->message);
			g_clear_error(&error);
		}
	}
}

static void proc_b25_chunk(B25Chunk *chunk)
{
	GTimeVal now;
	gdouble diff;

	g_get_current_time(&now);

	diff = now.tv_sec - chunk->arrived_time.tv_sec
		+ (((gdouble)now.tv_usec - chunk->arrived_time.tv_usec) / G_USEC_PER_SEC);
	if (diff >= 0 && diff < st_b25_ts_delay) {
		g_usleep(st_b25_ts_delay * G_USEC_PER_SEC);
	}

	proc_b25(chunk + 1, chunk->size);

	g_slice_free1(sizeof(B25Chunk) + chunk->size, chunk);
}

static gpointer
b25_thread(gpointer data)
{
	B25Chunk *chunk;

	g_async_queue_ref(st_b25_async_queue);

	while (st_is_b25_running) {
		while ((chunk = g_async_queue_try_pop(st_b25_async_queue))) {
			proc_b25_chunk(chunk);
		}

		if (!chunk) {
			g_usleep(1000);
			continue;
		}
	}

	while ((chunk = g_async_queue_try_pop(st_b25_async_queue))) {
		proc_b25_chunk(chunk);
	}

	g_async_queue_unref(st_b25_async_queue);

	return NULL;
}


/* Callbacks
   -------------------------------------------------------------------------- */
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
		GTimeVal now;
		B25Chunk *chunk;
			
		if (st_bcas_input_type == INPUT_TYPE_FX2) {
			PseudoBCASStatus status;
			((PSEUDO_B_CAS_CARD *)st_bcas)->get_status(st_bcas, &status);
			if (status.n_ecm_arrived == 0)
				return !st_is_intterupted;
		}

		g_get_current_time(&now);
			
		/* キューに積む */
		chunk = g_slice_alloc(sizeof(B25Chunk) + length);
		chunk->arrived_time = now;
		chunk->size = length;
		memcpy(chunk + 1, data, length);
		g_async_queue_push(st_b25_async_queue, chunk);
	}

	return !st_is_intterupted;
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

	return TRUE;
}

/* -------------------------------------------------------------------------- */

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
	GError *error = NULL;

	/* B-CAS カードクラスの初期化 */
	if (st_bcas_input_type == INPUT_TYPE_PCSC) {
#ifdef HAVE_LIBPCSCLITE
		g_message("*** using real B-CAS card reader");
		st_bcas = create_b_cas_card();
		is_pseudo_bcas = FALSE;
#else
		g_critical("!!! not build with libpcsclite");
		return FALSE;
#endif
	} else {
		st_bcas = (B_CAS_CARD *)pseudo_bcas_new();

		if (st_bcas_input_type == INPUT_TYPE_FX2) {
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
			return FALSE;
		}

		if (st_bcas_input_type == INPUT_TYPE_FX2) {
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
		return FALSE;
	}

	g_message("*** set MULTI-2 round factor to %d", st_b25_round);
	st_b25->set_multi2_round(st_b25, st_b25_round);

	g_message("*** %s omit NULL packets", st_b25_strip ? "Enable" : "Disable");
	st_b25->set_strip(st_b25, st_b25_strip ? 1 : 0);

	st_b25->set_b_cas_card(st_b25, st_bcas);

	/* Initialize B25 threads */
	g_thread_init(NULL);
	st_b25_thread = g_thread_create(b25_thread, NULL, TRUE, &error);
	if (error) {
		g_critical("[init_b25] %s", error->message);
		g_clear_error(&error);
		return FALSE;
	}
	st_b25_async_queue = g_async_queue_new();

	return TRUE;
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
	GTimer *timer; 
	gboolean is_cusbfx2_inited = FALSE;
	gboolean is_cusbfx2_started = FALSE;
	GString *infoline = NULL;
	gdouble ts_disposed_time;

	/* Initialize inputs */
	if (st_ts_input_type == INPUT_TYPE_FILE) {
		if (!(st_ts_input_io = open_io_channel(st_ts_input, FALSE))) {
			g_critical("!!! couldn't open TS input <%s>", st_ts_input);
			goto quit;
		}
	}

	if (st_bcas_input_type == INPUT_TYPE_FILE) {
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
#ifdef HAVE_LIBUSB
		cusbfx2_init();
		is_cusbfx2_inited = TRUE;

		device = capsts_open(st_fx2_id, st_fx2_is_force_load);
		if (!device) {
			goto quit;
		}

		capsts_cmd_push(CMD_PORT_CFG, 0x00, PIO_START);
		capsts_cmd_push(CMD_MODE_IDLE);
		capsts_cmd_push(CMD_IFCONFIG, 0xE3);
		capsts_cmd_commit(device);

		capsts_set_ir_base(st_ir_base);
		capsts_adjust_tuner_channel(device, st_ir_source, st_ir_channel);

		if (st_ts_input_type == INPUT_TYPE_FX2) {
			g_message("*** setup TS transfer");
			transfer_ts = cusbfx2_init_bulk_transfer(device, "TS", FALSE, ENDPOINT_TS_IN,
													 st_fx2_ts_buffer_size, st_fx2_ts_buffer_count,
													 transfer_ts_cb, NULL);
			if (!transfer_ts) {
				g_critical("!!! couldn't setup TS transfer");
				goto quit;
			}
			capsts_cmd_push(CMD_EP6IN_START);
		}

		if (st_bcas_input_type == INPUT_TYPE_FX2) {
			g_message("*** setup B-CAS transfer");
			transfer_bcas = cusbfx2_init_bulk_transfer(device, "B-CAS", TRUE, ENDPOINT_BCAS_IN,
													   512, 1, transfer_bcas_cb, NULL);
			if (!transfer_bcas) {
				g_critical("!!! couldn't setup B-CAS transfer");
				goto quit;
			}
			capsts_cmd_push(CMD_EP4IN_START);
		}

		capsts_cmd_push(CMD_PORT_WRITE, PIO_START);
		capsts_cmd_commit(device);
		is_cusbfx2_started = TRUE;

		if (transfer_ts) cusbfx2_start_transfer(transfer_ts);
		if (transfer_bcas) cusbfx2_start_transfer(transfer_bcas);
#endif
	}

	install_sighandler();

	/* B-CAS 入力がファイルであれば、事前に読んでおく */
	if (st_bcas_input_io) {
		for (;;) {
			GError *error = NULL;
			gchar buf[512];
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
				((PSEUDO_B_CAS_CARD *)st_bcas)->push(st_bcas, (guint8 *)buf, readed);
		}
	}

	/* main loop */
	infoline = g_string_sized_new(128);
	timer = g_timer_new();
	ts_disposed_time = -1.;
	while (!st_is_intterupted) {
		gdouble elapsed;

		if (st_ts_input_io) {
			GError *error = NULL;
			gchar buf[512];
			gsize readed;
			GIOStatus status;

			status = g_io_channel_read_chars(st_ts_input_io, buf, 512, &readed, &error);
			if (status == G_IO_STATUS_NORMAL) {
				transfer_ts_cb((guint8 *)buf, 512, NULL);
			} else {
				if (error) {
					g_warning("!!! %s", error->message);
					g_clear_error(&error);
				}
				break;
			}
		} else if (is_cusbfx2_started) {
#ifdef HAVE_LIBUSB
			PseudoBCASStatus bcas_status;
			if (st_bcas && (st_bcas_input_type == INPUT_TYPE_FX2 || st_bcas_input_type == INPUT_TYPE_FILE)) {
				((PSEUDO_B_CAS_CARD *)st_bcas)->get_status(st_bcas, &bcas_status);
			}

			cusbfx2_poll();

			elapsed = g_timer_elapsed(timer, NULL);

			if (st_b25_queue && ts_disposed_time < .0) {
				if (bcas_status.n_ecm_arrived > 0) {
					ts_disposed_time = elapsed;
					g_message("*** dispose leading TS stream by %.1f seconds", ts_disposed_time);
				}
			}

			g_string_printf(infoline, ">>> [Now] %.1f", elapsed);
			if (st_bcas && (st_bcas_input_type == INPUT_TYPE_FX2 || st_bcas_input_type == INPUT_TYPE_FILE)) {
				g_string_append_printf(infoline, " [ECM] fail:%d", bcas_status.n_ecm_failure);
			}
			if (st_b25_queue) {
				g_string_append_printf(infoline, " latency:%.3f-%.3f",
									   bcas_status.min_ecm_latecy, bcas_status.max_ecm_latecy);
				g_string_append_printf(infoline, " [TS] capacity:%.2fM(%3d%%)",
									   (gdouble)st_b25_queue_size / (1024 * 1024),
									   (gsize)((gdouble)st_b25_queue_size / MAX_B25_QUEUE_SIZE * 100));
			}
			if (!st_is_quiet) fprintf(stderr, "%s\r", infoline->str);

			if (st_length > 0 && elapsed > st_length) {
				break;
			}
#endif
		}
	}
	fprintf(stderr, "\n");
	if (infoline) g_string_free(infoline, TRUE);
	g_timer_destroy(timer);

	if (st_is_intterupted) {
		g_message("### INTERRUPTED SHUTDOWN ###");
	} else {
		g_message("### FINISHED SHUTDOWN ###");
	}

	/* TS転送を止め、対応するであろう鍵を受け取るまで待つ */
	if (st_b25_queue && st_bcas_input_type == INPUT_TYPE_FX2 && ts_disposed_time > .0) {
#ifdef HAVE_LIBUSB
		g_message("*** waiting for last ECM");
		if (transfer_ts) {
			capsts_cmd_push(CMD_EP6IN_STOP);
			capsts_cmd_commit(device);
			cusbfx2_free_transfer(transfer_ts);
			transfer_ts = NULL;
		}

		st_is_intterupted = FALSE;
		PseudoBCASStatus before_status, status;
		((PSEUDO_B_CAS_CARD *)st_bcas)->get_status(st_bcas, &before_status);
		status = before_status;
		while (!st_is_intterupted && status.n_ecm_arrived < before_status.n_ecm_arrived + 1) {
			cusbfx2_poll();
			((PSEUDO_B_CAS_CARD *)st_bcas)->get_status(st_bcas, &status);
		}

		if (transfer_bcas) {
			capsts_cmd_push(CMD_EP4IN_STOP);
			capsts_cmd_commit(device);
			cusbfx2_free_transfer(transfer_bcas);
			transfer_bcas = NULL;
		}
		g_message("*** waiting for last ECM");
#endif
	}

 quit:
	/* finalize */
	restore_sighandler();

	if (st_is_use_cusbfx2) {
#ifdef HAVE_LIBUSB
		if (is_cusbfx2_started) {
			g_message("*** set CUSBFX2 to idle mode");
			if (transfer_ts) capsts_cmd_push(CMD_EP6IN_STOP);
			if (transfer_bcas) capsts_cmd_push(CMD_EP4IN_STOP);
			capsts_cmd_push(CMD_MODE_IDLE);
			capsts_cmd_commit(device);
		}

		if (st_is_intterupted) {
			if (transfer_ts) cusbfx2_cancel_transfer(transfer_ts);
			if (transfer_bcas) cusbfx2_cancel_transfer(transfer_bcas);
		}
		if (transfer_ts) cusbfx2_free_transfer(transfer_ts);
		if (transfer_bcas) cusbfx2_free_transfer(transfer_bcas);

		if (device) cusbfx2_close(device);
		if (is_cusbfx2_inited) cusbfx2_exit();
#endif
	}

	/* flush */
	if (st_b25) {
		gint r;
		ARIB_STD_B25_BUFFER buffer;

		/* wait for b25 thread to finish ... */
		if (st_b25_thread) {
			st_is_b25_running = FALSE;
			g_thread_join(st_b25_thread);
		}

		g_message("*** flush B25 decoder");
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
			g_io_channel_write_chars(st_b25_output_io, (gchar *)buffer.data, buffer.size, &written, &error);
			if (error) {
				g_warning("!!! %s", error->message);
				g_clear_error(&error);
			}
		}

		info_b25(st_b25);

		if (st_b25) st_b25->release(st_b25);
		if (st_bcas) st_bcas->release(st_bcas);
	}

	if (st_b25_output_io) g_io_channel_shutdown(st_b25_output_io, TRUE, NULL);
	if (st_bcas_output_io) g_io_channel_shutdown(st_bcas_output_io, TRUE, NULL);
	if (st_ts_output_io) g_io_channel_shutdown(st_ts_output_io, TRUE, NULL);
	if (st_bcas_input_io) g_io_channel_shutdown(st_bcas_input_io, TRUE, NULL);
	if (st_ts_input_io) g_io_channel_shutdown(st_ts_input_io, TRUE, NULL);
}

static GString *
hexdump(guint8 *data, guint len, gboolean is_seperate)
{
	GString *result;
	guint i;

	result = g_string_sized_new(len * 3);
	for (i = 0; i < len; ++i) {
		g_string_append_printf(result, "%s%02x", ((i == 0 || !is_seperate) ? "" : " "), data[i]);
	}
	return result;
}

static void
dump_bcas_init_status()
{
	gint r;
	B_CAS_INIT_STATUS init;
	GString *hex;

#ifdef HAVE_LIBPCSCLITE
	st_bcas = create_b_cas_card();
	if (!st_bcas) {
		g_critical("!!! couldn't create B-CAS card reader");
		goto quit;
	}
#endif

	r = st_bcas->init(st_bcas);
	if (r < 0) {
		g_critical("!!! couldn't initialize B-CAS card reader (%d)", r);
		goto quit;
	}

	r = st_bcas->get_init_status(st_bcas, &init);
	if (r < 0) {
		g_critical("!!! couldn't get B-CAS init status (%d)", r);
		goto quit;
	}

	fprintf(stdout, "[b25]\n");

	hex = hexdump(init.system_key, 32, FALSE);
	fprintf(stdout, "system_key = %s\n", hex->str);
	g_string_free(hex, TRUE);

	hex = hexdump(init.init_cbc, 8, FALSE);
	fprintf(stdout, "init_cbc = %s\n", hex->str);
	g_string_free(hex, TRUE);

 quit:
	if (st_bcas) st_bcas->release(st_bcas);
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

	if (st_b25_ts_delay_string) {
		st_b25_ts_delay = g_ascii_strtod(st_b25_ts_delay_string, NULL);
	}

	if (g_str_has_prefix(st_ts_input, INPUT_TYPE_FX2_PREFIX))
		st_ts_input_type = INPUT_TYPE_FX2;
	else
		st_ts_input_type = INPUT_TYPE_FILE;

	if (g_str_has_prefix(st_bcas_input, INPUT_TYPE_FX2_PREFIX))
		st_bcas_input_type = INPUT_TYPE_FX2;
	else if (g_str_has_prefix(st_bcas_input, INPUT_TYPE_PCSC_PREFIX))
		st_bcas_input_type = INPUT_TYPE_PCSC;
	else
		st_bcas_input_type = INPUT_TYPE_FILE;

	if (st_ts_input_type == INPUT_TYPE_FX2 || st_bcas_input_type == INPUT_TYPE_FX2) {
		st_is_use_cusbfx2 = TRUE;
	}

#ifndef HAVE_LIBUSB
	if (st_is_use_cusbfx2) {
		g_critical("!!! not build with libusb");
		return FALSE;
	}
#endif

	if (!st_ts_output && !st_bcas_output && !st_b25_output && !st_dump_bcas_init_status) {
		return FALSE;
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
/* 		if (st_is_quiet) return; */
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

	fprintf(stderr, "%s,%03ld %s %s\n", asctime, now.tv_usec / 1000, level, message);
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

	if (st_dump_bcas_init_status) {
		dump_bcas_init_status();
	} else if (st_verify_bcas_stream > 0) {
		verify_bcas_stream();
	} else {
		run();
	}

	return 0;
}
