bin_PROGRAMS = tsniff

tsniff_SOURCES = main.c
tsniff_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/lib
tsniff_LDADD = $(top_builddir)/lib/libcapsts.la @PCSCLITE_LIBS@
#libcapsts_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/extra/libusb/libusb/libusb-1.0.la
