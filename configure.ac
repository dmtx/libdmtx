AC_INIT([libdmtx], [0.5.2], [mike@dragonflylogic.com])
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
AM_CONDITIONAL([TARGET_MACOSX], [test x$ARCH = xmacosx])

jpeg_ok=no
AC_SEARCH_LIBS([jpeg_read_scanlines], [jpeg],
   [AC_CHECK_HEADER([jpeglib.h],
      [jpeg_ok=yes],
      [AC_MSG_WARN([*** JPEG support will not be included (header not found) ***])]
   )],
   [AC_MSG_WARN([*** JPEG support will not be included (library not found) ***])]
)

if test x$jpeg_ok = xyes; then
   AC_DEFINE([HAVE_LIBJPEG], [1], [Define to 1 if you have the libjpeg header and library.])
fi
AM_CONDITIONAL([HAVE_LIBJPEG], [test x$jpeg_ok = xyes])

tiff_ok=no
AC_SEARCH_LIBS([TIFFReadRGBAImage], [tiff],
   [AC_CHECK_HEADER([tiff.h],
      [tiff_ok=yes],
      [AC_MSG_WARN([*** TIFF support will not be included (header not found) ***])]
   )],
   [AC_MSG_WARN([*** TIFF support will not be included (library not found) ***])]
)

if test x$tiff_ok = xyes; then
   AC_DEFINE([HAVE_LIBTIFF], [1], [Define to 1 if you have the libtiff header and library.])
fi
AM_CONDITIONAL([HAVE_LIBTIFF], [test x$tiff_ok = xyes])

AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_FUNCS([gettimeofday])

AC_CHECK_HEADERS([getopt.h])
AC_CHECK_FUNCS([getopt_long])

AC_OUTPUT
