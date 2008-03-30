noinst_LIBRARIES = libcapsts.a

libcapsts_a_SOURCES = \
	bcas_stream.c \
	bcas_stream.h \
	bcas_card_streaming.c \
	bcas_card_streaming.h \
	capsts.c \
	capsts.h \
	cusbfx2.c \
	cusbfx2.h \
	fw.inc
libcapsts_a_CFLAGS = -I$(top_srcdir)/extra/b25/src -I$(top_srcdir)/extra/libusb/libusb @GLIB_CFLAGS@
#libcapsts_a_LIBADD = 