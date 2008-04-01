bin_PROGRAMS = tsniff

capsts_libs = $(top_builddir)/lib/libcapsts.a $(top_builddir)/extra/b25/src/libb25.a $(top_builddir)/extra/libusb/libusb/.libs/libusb-1.0.a @PCSCLITE_LIBS@ @GLIB_LIBS@

tsniff_SOURCES = main.c
tsniff_CFLAGS = -I$(top_srcdir)/extra/b25/src @GLIB_CFLAGS@ -I$(top_srcdir)/lib
tsniff_LDADD = $(capsts_libs)
