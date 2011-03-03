AC_INIT([libdmtx], [0.7.3], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
   Makefile
   test/Makefile
   test/multi_test/Makefile
   test/rotate_test/Makefile
   test/simple_test/Makefile
   test/unit_test/Makefile
])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

AC_SEARCH_LIBS([sin], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([cos], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))
AC_SEARCH_LIBS([atan2], [m] ,[], AC_MSG_ERROR([libdmtx requires libm]))

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

AC_CONFIG_FILES([libdmtx.pc])

AC_OUTPUT
