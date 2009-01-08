AC_INIT([libdmtx], [0.6.1], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
   util/Makefile
   test/Makefile
   test/multi_test/Makefile
   test/rotate_test/Makefile
   test/simple_test/Makefile
   test/unit_test/Makefile
])

AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([cos], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([atan2], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

case $host_os in
   cygwin*)
      ARCH=cygwin ;;
   darwin*)
      ARCH=macosx ;;
   freebsd*)
      ARCH=freebsd ;;
   linux-gnu*)
      ARCH=linux-gnu ;;
   mingw32*)
      ARCH=mingw32 ;;
esac
AM_CONDITIONAL([TARGET_MACOSX], [test x$ARCH = xmacosx])

AC_ARG_ENABLE(
   dmtxquery,
   AC_HELP_STRING(
      [--disable-dmtxquery],
      [Do not build the dmtxquery command line utility]),
   [case "$enableval" in
      yes) dmtxquery=true ;;
       no) dmtxquery=false ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-dmtxquery]) ;;
    esac],
   [dmtxquery=true])
AM_CONDITIONAL([BUILD_DMTXQUERY], [test x$dmtxquery = xtrue])

AC_ARG_ENABLE(
   dmtxread,
   AC_HELP_STRING(
      [--disable-dmtxread],
      [Do not build the dmtxread command line utility (removes GraphicsMagick dependency)]),
   [case "$enableval" in
      yes) dmtxread=true ;;
       no) dmtxread=false ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-dmtxread]) ;;
    esac],
   [dmtxread=true])
AM_CONDITIONAL([BUILD_DMTXREAD], [test x$dmtxread = xtrue])

AC_ARG_ENABLE(
   dmtxwrite,
   AC_HELP_STRING(
      [--disable-dmtxwrite],
      [Do not build the dmtxwrite command line utility (removes libpng dependency)]),
   [case "$enableval" in
      yes) dmtxwrite=true ;;
       no) dmtxwrite=false ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-dmtxwrite]) ;;
    esac],
   [dmtxwrite=true])
AM_CONDITIONAL([BUILD_DMTXWRITE], [test x$dmtxwrite = xtrue])

if test x$dmtxquery = xtrue; then
   AC_CONFIG_FILES([util/dmtxquery/Makefile])
fi

if test x$dmtxread = xtrue; then
   ifdef([PKG_CHECK_MODULES],
         [PKG_CHECK_MODULES(
            MAGICK,
            GraphicsMagick >= 1.1.7,
            true,
            AC_MSG_ERROR([[dmtxread requires GraphicsMagick >= 1.1.7]]))],
         [AC_MSG_ERROR([Building dmtxread requires a working autoconf/pkg-config setup])])
   AC_SUBST(MAGICK_CFLAGS)
   AC_SUBST(MAGICK_LIBS)
   AC_CONFIG_FILES([util/dmtxread/Makefile])
fi

if test x$dmtxwrite = xtrue; then
   ifdef([PKG_CHECK_MODULES],
         [PKG_CHECK_MODULES(
            PNG,
            libpng >= 1.0.0,
            true,
            AC_MSG_ERROR([[dmtxread requires libpng >= 1.0.0]]))],
         [AC_MSG_ERROR([Building dmtxwrite requires a working autoconf/pkg-config setup])])
   AC_SUBST(PNG_CFLAGS)
   AC_SUBST(PNG_LIBS)
   AC_CONFIG_FILES([util/dmtxwrite/Makefile])
fi

if test x$dmtxread = xtrue -o x$dmtxwrite = xtrue -o x$dmtxquery = xtrue; then
   AC_CHECK_HEADERS([sysexits.h])
   AC_CHECK_HEADERS([getopt.h])
   AC_CHECK_FUNCS([getopt_long])
fi

AC_OUTPUT
