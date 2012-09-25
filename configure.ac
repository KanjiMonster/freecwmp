AC_INIT([freecwmp], [0.1], [freecwmp@lukaperkov.net])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/freecwmp.c])

# additional options
AC_ARG_ENABLE(acs, [AS_HELP_STRING([--enable-acs],
	[specify which ACS is going to be used; there are some optimizations available for HDM (default --enable-acs=multi]))],,
	[AC_DEFINE(ACS_MULTI)])

AS_IF([test "x$enable_acs" = "xhdm"], [AC_DEFINE(ACS_HDM)])
AS_IF([test "x$enable_acs" = "xmulti"], [AC_DEFINE(ACS_MULTI)])
AS_IF([test "x$enable_acs" = "xyes"], [AC_DEFINE(ACS_MULTI)])
AS_IF([test "x$enable_acs" = "xno"], [AC_DEFINE(ACS_MULTI)])

AC_ARG_ENABLE(http, [AS_HELP_STRING([--enable-http],
	[specify which HTTP library is going to be used; one can choose from zstream and curl (default --enable-http=curl]))],,)

AM_CONDITIONAL([HTTP_ZSTREAM], [test "x$enable_http" = "xzstream" ])
AM_CONDITIONAL([HTTP_CURL], [test "x$enable_http" = "xcurl" ])

AC_ARG_ENABLE(debug, [AS_HELP_STRING([--enable-debug], [enable debugging messages])], AC_DEFINE(DEBUG),)
AC_ARG_ENABLE(devel, [AS_HELP_STRING([--enable-devel], [enable development messages])], AC_DEFINE(DEVEL),)
AC_ARG_ENABLE(dummy_mode, [AS_HELP_STRING([--enable-dummy-mode], [enable development mode])], AC_DEFINE(DUMMY_MODE),)

# checks for programs
AC_PROG_CC
AM_PROG_CC_C_O

# checks for libraries
AC_ARG_WITH([libfreecwmp-include-path],
  [AS_HELP_STRING([--with-libfreecwmp-include-path],
    [location of the libfreecwmp library headers, defaults to /usr/include])],
  [LIBFREECWMP_CFLAGS="-I$withval"],
  [LIBFREECWMP_CFLAGS='-I/usr/include'])
AC_SUBST([LIBFREECWMP_CFLAGS])

AC_ARG_WITH([libfreecwmp-lib-path],
  [AS_HELP_STRING([--with-libfreecwmp-lib-path], [location of the libfreecwmp library])], [LIBFREECWMP_LDFLAGS="-L$withval"])
AC_SUBST([LIBFREECWMP_LDFLAGS])

LIBFREECWMP_LIBS='-lfreecwmp'
AC_SUBST([LIBFREECWMP_LIBS])

AC_ARG_WITH([uci-include-path],
  [AS_HELP_STRING([--with-uci-include-path],
    [location of the uci library headers, defaults to /usr/include])],
  [LIBUCI_CFLAGS="-I$withval"],
  [LIBUCI_CFLAGS='-I/usr/include'])
AC_SUBST([LIBUCI_CFLAGS])

AC_ARG_WITH([uci-lib-path],
  [AS_HELP_STRING([--with-uci-lib-path], [location of the uci library])], [LIBUCI_LDFLAGS="-L$withval"])
AC_SUBST([LIBUCI_LDFLAGS])

LIBUCI_LIBS='-luci'
AC_SUBST([LIBUCI_LIBS])

AC_ARG_WITH([libubox-include-path],
  [AS_HELP_STRING([--with-libubox-include-path],
    [location of the libubox library headers, defaults to /usr/include])],
  [LIBUBOX_CFLAGS="-I$withval"],
  [LIBUBOX_CFLAGS='-I/usr/include'])
AC_SUBST([LIBUBOX_CFLAGS])

AC_ARG_WITH([libubox-lib-path],
  [AS_HELP_STRING([--with-libubox-lib-path], [location of the libubox library])], [LIBUBOX_LDFLAGS="-L$withval"])
AC_SUBST([LIBUBOX_LDFLAGS])

LIBUBOX_LIBS='-lubox'
AC_SUBST([LIBUBOX_LIBS])

AC_ARG_WITH([libubus-include-path],
  [AS_HELP_STRING([--with-libubus-include-path],
    [location of the libubus library headers, defaults to /usr/include])],
  [LIBUBUS_CFLAGS="-I$withval"],
  [LIBUBUS_CFLAGS='-I/usr/include'])
AC_SUBST([LIBUBUS_CFLAGS])

AC_ARG_WITH([libubus-lib-path],
  [AS_HELP_STRING([--with-libubus-lib-path], [location of the libubus library])], [LIBUBOX_LDFLAGS="-L$withval"])
AC_SUBST([LIBUBUS_LDFLAGS])

LIBUBUS_LIBS='-lubus'
AC_SUBST([LIBUBUS_LIBS])

AM_COND_IF([HTTP_CURL], [
 AC_DEFINE(HTTP_CURL)
 PKG_CHECK_MODULES(LIBCURL, [libcurl])
 AC_SUBST(LIBCURL_CFLAGS)
 AC_SUBST(LIBCURL_LDFLAGS)
 AC_SUBST(LIBCURL_LIBS)
])

AM_COND_IF([HTTP_ZSTREAM], [
 AC_DEFINE(HTTP_ZSTREAM)
 AC_ARG_WITH([zstream-include-path],
   [AS_HELP_STRING([--with-zstream-include-path],
     [location of the zstream library headers, defaults to /usr/include])],
   [LIBZSTREAM_CFLAGS="-I$withval"],
   [LIBZSTREAM_CFLAGS='-I/usr/include'])
 AC_SUBST([LIBZSTREAM_CFLAGS])

 AC_ARG_WITH([zstream-lib-path],
   [AS_HELP_STRING([--with-zstream-lib-path], [location of the zstream library])], [LIBZSTREAM_LDFLAGS="-L$withval"])

 LIBZSTREAM_LIBS='-lzstream'
 AC_SUBST([LIBZSTREAM_LIBS])
])

PKG_CHECK_MODULES(MICROXML, [microxml])
AC_SUBST(MICROXML_CFLAGS)
AC_SUBST(MICROXML_LDFLAGS)
AC_SUBST(MICROXML_LIBS)

# checks for header files
AC_CHECK_HEADERS([stdlib.h string.h])

# checks for typedefs, structures, and compiler characteristics
AC_TYPE_UINT8_T

# Makefiles
AC_CONFIG_FILES([
Makefile
bin/Makefile
])

AC_OUTPUT
