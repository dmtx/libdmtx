AC_INIT([libdmtx], [0.5.1], [mike@dragonflylogic.com])
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
])

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNCS([getopt_long])

AC_OUTPUT
