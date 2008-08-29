AC_INIT([libdmtx], [0.6.0], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_PROG_CC
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 util/Makefile
 util/dmtxquery/Makefile
 util/dmtxread/Makefile
 util/dmtxwrite/Makefile
 test/Makefile
 test/test_opengl/Makefile
 test/test_simple/Makefile
 test/test_unit/Makefile
])

case $host_os in
   darwin*)
      ARCH=macosx
      ;;
esac
AM_CONDITIONAL(TARGET_MACOSX, test "$ARCH" = macosx)

AC_CHECK_HEADER([tiff.h],
   AC_CHECK_LIB([tiff], [TIFFReadScanline],
      AC_DEFINE([HAVE_LIBTIFF], [1], [Define to 1 if you have the libtiff header and library.])))

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNCS([getopt_long])

AC_OUTPUT
