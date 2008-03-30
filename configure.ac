#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(tsniff, 0.0.1, atty303@gmail.com)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([tsniff/main.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([rt], [clock_gettime])

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.14)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(PCSCLITE, libpcsclite)
AC_SUBST(PCSCLITE_CFLAGS)
AC_SUBST(PCSCLITE_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MEMCMP

AC_CONFIG_SUBDIRS([extra/libusb extra/b25])
AC_CONFIG_FILES([Makefile lib/Makefile extra/Makefile tsniff/Makefile])
AC_OUTPUT
