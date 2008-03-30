bin_PROGRAMS = tsniff bcasdump b25decode

capsts_libs = $(top_builddir)/lib/libcapsts.a $(top_builddir)/extra/b25/src/libb25.a $(top_builddir)/extra/libusb/libusb/.libs/libusb-1.0.a @PCSCLITE_LIBS@ @GLIB_LIBS@

tsniff_SOURCES = main.c
tsniff_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/lib
tsniff_LDADD = $(capsts_libs)
#libcapsts_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/extra/libusb/libusb/libusb-1.0.la

bcasdump_SOURCES = bcasdump.c
bcasdump_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/lib
bcasdump_LDADD = $(capsts_libs)

b25decode_SOURCES = b25decode.c
b25decode_CFLAGS = -I$(top_srcdir)/extra/b25/src @GLIB_CFLAGS@ -I$(top_srcdir)/lib
b25decode_LDADD = $(capsts_libs)
