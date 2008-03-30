bin_PROGRAMS = tsniff bcasdump b25decode

tsniff_SOURCES = main.c
tsniff_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/lib
tsniff_LDADD = $(top_builddir)/lib/libcapsts.la @PCSCLITE_LIBS@
#libcapsts_la_LIBADD = @GLIB_LIBS@ $(top_builddir)/extra/libusb/libusb/libusb-1.0.la

bcasdump_SOURCES = bcasdump.c
bcasdump_CFLAGS = @GLIB_CFLAGS@ -I$(top_srcdir)/lib
bcasdump_LDADD = $(top_builddir)/lib/libcapsts.la @B25_LIBS@ @PCSCLITE_LIBS@

b25decode_SOURCES = b25decode.c
b25decode_CFLAGS = @B25_CFLAGS@ @GLIB_CFLAGS@ -I$(top_srcdir)/lib
b25decode_LDADD = $(top_builddir)/lib/libcapsts.la @B25_LIBS@ @PCSCLITE_LIBS@
