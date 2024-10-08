AC_INIT([libdmtx], [0.7.8], [https://github.com/dmtx/libdmtx/issues], [libdmtx], [https://github.com/dmtx/libdmtx])
AM_INIT_AUTOMAKE([-Wall -Werror])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
   libdmtx.pc
   test/Makefile
   test/simple_test/Makefile
])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O


AC_ARG_ENABLE([reader_programming], [AS_HELP_STRING([--enable-reader-programming], [Enable Reader Programming breaking feature.])], [], [enable_reader_programming=no] )
AS_IF([test "x$enable_reader_programming" = "xyes"], [
  AC_DEFINE([HAVE_READER_PROGRAMMING], [1], [Reader Programming breaking feature.])
])


AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([cos], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([atan2], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

case $target_os in
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

AC_OUTPUT
