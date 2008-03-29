noinst_LTLIBRARIES = libcapsts.la

libcapsts_la_SOURCES = \
	capsts.c \
	capsts.h \
	cusbfx2.c \
	cusbfx2.h \
	fw.inc
libcapsts_la_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/extra/libusb/libusb
libcapsts_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/extra/libusb/libusb/libusb-1.0.la
