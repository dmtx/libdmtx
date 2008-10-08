AC_INIT([libdmtx], [0.5.3], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

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
AM_CONDITIONAL([TARGET_MACOSX], [test x$ARCH = xmacosx])

PKG_CHECK_MODULES(MAGICK, GraphicsMagick >= 1.1.7, true,
                  AC_MSG_ERROR([you need GraphicsMagick >= 1.1.7]))
AC_SUBST(MAGICK_CFLAGS)
AC_SUBST(MAGICK_LIBS)

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNCS([getopt_long])

AC_OUTPUT
