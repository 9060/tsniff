noinst_LTLIBRARIES = libcapsts.la

libcapsts_la_SOURCES = \
	bcas_stream.c \
	bcas_stream.h \
	bcas_fx2stream.c \
	bcas_fx2stream.h \
	capsts.c \
	capsts.h \
	cusbfx2.c \
	cusbfx2.h \
	fw.inc
libcapsts_la_CFLAGS = @B25_CFLAGS@ @GLIB_CFLAGS@ -I$(top_srcdir)/extra/libusb/libusb
libcapsts_la_LIBADD = @B25_LIBS@ @GLIB_LIBS@ $(top_builddir)/extra/libusb/libusb/libusb-1.0.la
