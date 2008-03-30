#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "arib_std_b25.h"
#include "b_cas_card.h"
#include "bcas_card_streaming.h"

static gchar *st_output_filename = NULL;

static GOptionEntry st_options[] = {
	{ "output-filename", 'o', 0, G_OPTION_ARG_FILENAME, &st_output_filename, "Output decoded stream to FILENAME", "FILENAME" },
	{ NULL }
};

gboolean
preload_bcas(B_CAS_CARD *bcas, const gchar *filename)
{
	gchar buf[512];
	gsize readed;
	GIOChannel *io_bcas = NULL;
	GError *error = NULL;

	io_bcas = g_io_channel_new_file(filename, "r", &error);
	if (!io_bcas) {
		return FALSE;
	}

	g_io_channel_set_encoding(io_bcas, NULL, &error);
	while (g_io_channel_read_chars(io_bcas, buf, 512, &readed, &error) == G_IO_STATUS_NORMAL) {
		bcas_card_streaming_push(bcas, buf, readed);
	}

	return TRUE;
}

int
main(int argc, char **argv)
{
	GOptionContext *context;
	GError *error = NULL;
	ARIB_STD_B25 *b25;
	B_CAS_CARD *bcas;
	GIOChannel *io_ts = NULL, *io_ts_out;
	gchar buf[16384];
	gsize readed;

	context = g_option_context_new("- B25 Decoder");
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, st_options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		return 1;
	}

	g_log_set_handler(NULL, G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION,
					  g_log_default_handler, NULL);

	if (argc != 3 || !st_output_filename) {
		return 1;
	}

	b25 = create_arib_std_b25();
	if (!b25) {
		return 1;
	}
	b25->set_multi2_round(b25, 4);
	b25->set_strip(b25, 0);

	bcas = bcas_card_streaming_new();
	bcas->init(bcas);
	bcas_card_streaming_set_queue_len(bcas, G_MAXUINT);
	if (!preload_bcas(bcas, argv[2]))
		return 1;
	b25->set_b_cas_card(b25, bcas);

	/* initialize */
	io_ts = g_io_channel_new_file(argv[1], "r", &error);
	if (!io_ts) {
		return 1;
	}
	g_io_channel_set_encoding(io_ts, NULL, &error);

	io_ts_out = g_io_channel_new_file(st_output_filename, "w", &error);
	if (!io_ts_out) {
		return 1;
	}
	g_io_channel_set_encoding(io_ts_out, NULL, &error);

	/* decode */
	while (g_io_channel_read_chars(io_ts, buf, 16384, &readed, &error) == G_IO_STATUS_NORMAL) {
		ARIB_STD_B25_BUFFER src_buf, dst_buf;
		src_buf.data = buf;
		src_buf.size = readed;

		if (b25->put(b25, &src_buf) < 0) {
			return 1;
		}
		if (b25->get(b25, &dst_buf) < 0) {
			return 1;
		}

		if (dst_buf.size > 0) {
			g_io_channel_write_chars(io_ts_out, dst_buf.data, dst_buf.size, NULL, &error);
		}
	}

	b25->flush(b25);

	/* finalize */
 quit:
	g_io_channel_shutdown(io_ts_out, TRUE, &error);
	g_io_channel_shutdown(io_ts, TRUE, &error);
	if (b25) b25->release(b25);
	if (bcas) bcas->release(bcas);

	return 0;
}
