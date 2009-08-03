AC_INIT([libdmtx], [0.7.1], [mike@dragonflylogic.com])
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
   wrapper/Makefile
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
      [Do not build the dmtxread command line utility]),
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
      [Do not build the dmtxwrite command line utility]),
   [case "$enableval" in
      yes) dmtxwrite=true ;;
       no) dmtxwrite=false ;;
        *) AC_MSG_ERROR([bad value ${enableval} for --enable-dmtxwrite]) ;;
    esac],
   [dmtxwrite=true])
AM_CONDITIONAL([BUILD_DMTXWRITE], [test x$dmtxwrite = xtrue])

if test x$dmtxread = xtrue \
      -o x$dmtxwrite = xtrue \
      -o x$dmtxquery = xtrue; then
   AC_CHECK_HEADERS([sysexits.h])
   AC_CHECK_HEADERS([getopt.h])
   AC_CHECK_FUNCS([getopt_long])
fi

if test x$dmtxread = xtrue \
      -o x$dmtxwrite = xtrue; then
   ifdef([PKG_CHECK_MODULES],
         [PKG_CHECK_MODULES(
            MAGICK,
            Wand >= 6.2.4,
            true,
            AC_MSG_ERROR([[dmtxread/dmtxwrite requires Wand >= 6.2.4]]))],
         [AC_MSG_ERROR([Building dmtxread/dmtxwrite requires a working autoconf/pkg-config])])
   AC_SUBST(MAGICK_CFLAGS)
   AC_SUBST(MAGICK_LIBS)
fi

if test x$dmtxquery = xtrue; then
   AC_CONFIG_FILES([util/dmtxquery/Makefile])
fi

if test x$dmtxread = xtrue; then
   AC_CONFIG_FILES([util/dmtxread/Makefile])
fi

if test x$dmtxwrite = xtrue; then
   AC_CONFIG_FILES([util/dmtxwrite/Makefile])
fi

# Create options to override certain functions

# --enable-customEncodeDataMatrix
AC_ARG_ENABLE([customEncodeDataMatrix], [],
              [customEncodeDataMatrix="$enableval"],
              [customEncodeDataMatrix="no"])
AM_CONDITIONAL([CUSTOM_ENCODEDATAMATRIX], [test x$customEncodeDataMatrix = xyes])
if test x$customEncodeDataMatrix = xyes; then
   AC_DEFINE([CUSTOM_ENCODEDATAMATRIX], [1], [Override dmtxEncodeDataMatrix function])
fi

# --enable-customEncodeDataMosaic
AC_ARG_ENABLE([customEncodeDataMosaic], [],
              [customEncodeDataMosaic="$enableval"],
              [customEncodeDataMosaic="no"])
AM_CONDITIONAL([CUSTOM_ENCODEDATAMOSAIC], [test x$customEncodeDataMosaic = xyes])
if test x$customEncodeDataMosaic = xyes; then
   AC_DEFINE([CUSTOM_ENCODEDATAMOSAIC], [1], [Override customEncodeDataMosaic function])
fi

# --enable-customDecodeMatrixRegion
AC_ARG_ENABLE([customDecodeMatrixRegion], [],
              [customDecodeMatrixRegion="$enableval"],
              [customDecodeMatrixRegion="no"])
AM_CONDITIONAL([CUSTOM_DECODEMATRIXREGION], [test x$customDecodeMatrixRegion = xyes])
if test x$customDecodeMatrixRegion = xyes; then
   AC_DEFINE([CUSTOM_DECODEMATRIXREGION], [1], [Override customDecodeMatrixRegion function])
fi

# --enable-customDecodeMosaicRegion
AC_ARG_ENABLE([customDecodeMosaicRegion], [],
              [customDecodeMosaicRegion="$enableval"],
              [customDecodeMosaicRegion="no"])
AM_CONDITIONAL([CUSTOM_DECODEMOSAICREGION], [test x$customDecodeMosaicRegion = xyes])
if test x$customDecodeMosaicRegion = xyes; then
   AC_DEFINE([CUSTOM_DECODEMOSAICREGION], [1], [Override customDecodeMosaicRegion function])
fi

# --enable-customRegionFindNext
AC_ARG_ENABLE([customRegionFindNext], [],
              [customRegionFindNext="$enableval"],
              [customRegionFindNext="no"])
AM_CONDITIONAL([CUSTOM_REGIONFINDNEXT], [test x$customRegionFindNext = xyes])
if test x$customRegionFindNext = xyes; then
   AC_DEFINE([CUSTOM_REGIONFINDNEXT], [1], [Override customRegionFindNext function])
fi

# --enable-customRegionScanPixel
AC_ARG_ENABLE([customRegionScanPixel], [],
              [customRegionScanPixel="$enableval"],
              [customRegionScanPixel="no"])
AM_CONDITIONAL([CUSTOM_REGIONSCANPIXEL], [test x$customRegionScanPixel = xyes])
if test x$customRegionScanPixel = xyes; then
   AC_DEFINE([CUSTOM_REGIONSCANPIXEL], [1], [Override customRegionScanPixel function])
fi

AC_ARG_ENABLE([cocoa],
              AS_HELP_STRING([--enable-cocoa], [enable Cocoa bindings]),
              [enable_cocoa="$enableval"],
              [enable_cocoa="no"])
if test x$enable_cocoa = xyes; then
   AC_WARN([Building the Cocoa wrapper though the libdmtx build system is not yet supported])
fi

AM_CONDITIONAL(ENABLE_COCOA, [test x$enable_cocoa = xyes])

AC_ARG_ENABLE([java],
              AS_HELP_STRING([--enable-java], [enable Java bindings]),
              [enable_java="$enableval"],
              [enable_java="no"])
if test x$enable_java = xyes; then
   AC_WARN([Building the Java wrapper though the libdmtx build system is not yet supported])
fi

AM_CONDITIONAL(ENABLE_JAVA, [test x$enable_java = xyes])

AC_ARG_ENABLE([net],
              AS_HELP_STRING([--enable-net], [enable .Net bindings]),
              [enable_net="$enableval"],
              [enable_net="no"])
if test x$enable_net = xyes; then
   AC_WARN([Building the .Net wrapper though the libdmtx build system is not yet supported])
fi

AM_CONDITIONAL(ENABLE_NET, [test x$enable_net = xyes])

AC_ARG_ENABLE([php],
              AS_HELP_STRING([--enable-php], [enable PHP bindings]),
              [enable_php="$enableval"],
              [enable_php="no"])
if test x$enable_php = xyes; then
   AC_PATH_PROG([PHPIZE], [phpize])
   if test ! -e "$PHPIZE"; then
     AC_ERROR([phpize is required to generate the PHP bindings.])
   fi

   AC_WARN([Building the PHP wrapper will break make uninstall])

   dmtx_dir=`pwd`
   cd wrapper/php && \
   $PHPIZE && \
   ./configure --prefix=${prefix} && \
   cd $dmtx_dir
fi

AM_CONDITIONAL([ENABLE_PHP], [test x$enable_php = xyes])

AC_ARG_ENABLE([python],
              AS_HELP_STRING([--enable-python], [enable Python bindings]),
              [enable_python="$enableval"],
              [enable_python="no"])
if test x$enable_python = xyes; then
   AM_PATH_PYTHON
   AC_WARN([Building the Ruby wrapper will break make uninstall])
fi

AM_CONDITIONAL(ENABLE_PYTHON, [test x$enable_python = xyes])

AC_ARG_ENABLE([ruby],
              AS_HELP_STRING([--enable-ruby], [enable Ruby bindings]),
              [enable_ruby="$enableval"],
              [enable_ruby="no"])
if test x$enable_ruby = xyes; then
   AC_PATH_PROG([RUBY], [ruby])
   if test ! -e "$RUBY"; then
     AC_ERROR([ruby is required to generate the Ruby bindings.])
   fi

   AC_WARN([Building the Ruby wrapper will break make uninstall])

   dmtx_dir=`pwd`
   cd wrapper/ruby && \
   $RUBY extconf.rb --with-dmtx-dir=${prefix}
   cd $dmtx_dir
fi

AM_CONDITIONAL(ENABLE_RUBY, [test x$enable_ruby = xyes])

AC_CONFIG_FILES([libdmtx.pc])

AC_OUTPUT
