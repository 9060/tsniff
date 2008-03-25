#include <glib.h>
#include "cusbfx2.h"

static GOptionEntry st_options[] = {
	{ "repeats", 'r', 0, G_OPTION_ARG_INT, &repeats, "Average over N repetitions", "N" },
	{ "max-size", 'm', 0, G_OPTION_ARG_INT, &max_size, "Test up to 2^M items", "M" },
	{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL },
	{ "beep", 'b', 0, G_OPTION_ARG_NONE, &beep, "Beep when done", NULL },
	{ "rand", 0, 0, G_OPTION_ARG_NONE, &rand, "Randomize the data", NULL },
	{ NULL }
};

int
main(int argc, char **argv)
{
	GError *error;
	GOptionContext *context;

	context = g_option_context_new("- CUSBFX2 TS Recorder");
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, st_options, NULL);
	if (!g_option_context_parse(context, &argc, &argv, &error)) {
		exit(1);
	}
}
