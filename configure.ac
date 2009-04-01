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

# Create option to override certain functions with custom versions

# --enable-customEncodeDataMatrix
AC_ARG_ENABLE(
   [customEncodeDataMatrix],
   [],
   [case "$enableval" in
      yes) customEncodeDataMatrix=true ;;
       no) customEncodeDataMatrix=false ;;
    esac],
   [customEncodeDataMatrix=false])
AM_CONDITIONAL([CUSTOM_ENCODEDATAMATRIX], [test x$customEncodeDataMatrix = xtrue])
if test x$customEncodeDataMatrix = xtrue; then
   AC_DEFINE([CUSTOM_ENCODEDATAMATRIX], [1], [Override dmtxEncodeDataMatrix function])
fi

# --enable-customEncodeDataMosaic
AC_ARG_ENABLE(
   [customEncodeDataMosaic],
   [],
   [case "$enableval" in
      yes) customEncodeDataMosaic=true ;;
       no) customEncodeDataMosaic=false ;;
    esac],
   [customEncodeDataMosaic=false])
AM_CONDITIONAL([CUSTOM_ENCODEDATAMOSAIC], [test x$customEncodeDataMosaic = xtrue])
if test x$customEncodeDataMosaic = xtrue; then
   AC_DEFINE([CUSTOM_ENCODEDATAMOSAIC], [1], [Override customEncodeDataMosaic function])
fi

# --enable-customDecodeMatrixRegion
AC_ARG_ENABLE(
   [customDecodeMatrixRegion],
   [],
   [case "$enableval" in
      yes) customDecodeMatrixRegion=true ;;
       no) customDecodeMatrixRegion=false ;;
    esac],
   [customDecodeMatrixRegion=false])
AM_CONDITIONAL([CUSTOM_DECODEMATRIXREGION], [test x$customDecodeMatrixRegion = xtrue])
if test x$customDecodeMatrixRegion = xtrue; then
   AC_DEFINE([CUSTOM_DECODEMATRIXREGION], [1], [Override customDecodeMatrixRegion function])
fi

# --enable-customDecodeMosaicRegion
AC_ARG_ENABLE(
   [customDecodeMosaicRegion],
   [],
   [case "$enableval" in
      yes) customDecodeMosaicRegion=true ;;
       no) customDecodeMosaicRegion=false ;;
    esac],
   [customDecodeMosaicRegion=false])
AM_CONDITIONAL([CUSTOM_DECODEMOSAICREGION], [test x$customDecodeMosaicRegion = xtrue])
if test x$customDecodeMosaicRegion = xtrue; then
   AC_DEFINE([CUSTOM_DECODEMOSAICREGION], [1], [Override customDecodeMosaicRegion function])
fi

# --enable-customDecodeCreateDiagnostic
AC_ARG_ENABLE(
   [customDecodeCreateDiagnostic],
   [],
   [case "$enableval" in
      yes) customDecodeCreateDiagnostic=true ;;
       no) customDecodeCreateDiagnostic=false ;;
    esac],
   [customDecodeCreateDiagnostic=false])
AM_CONDITIONAL([CUSTOM_DECODECREATEDIAGNOSTIC], [test x$customDecodeCreateDiagnostic = xtrue])
if test x$customDecodeCreateDiagnostic = xtrue; then
   AC_DEFINE([CUSTOM_DECODECREATEDIAGNOSTIC], [1], [Override customDecodeCreateDiagnostic function])
fi

# --enable-customRegionFindNext
AC_ARG_ENABLE(
   [customRegionFindNext],
   [],
   [case "$enableval" in
      yes) customRegionFindNext=true ;;
       no) customRegionFindNext=false ;;
    esac],
   [customRegionFindNext=false])
AM_CONDITIONAL([CUSTOM_REGIONFINDNEXT], [test x$customRegionFindNext = xtrue])
if test x$customRegionFindNext = xtrue; then
   AC_DEFINE([CUSTOM_REGIONFINDNEXT], [1], [Override customRegionFindNext function])
fi

# --enable-customRegionScanPixel
AC_ARG_ENABLE(
   [customRegionScanPixel],
   [],
   [case "$enableval" in
      yes) customRegionScanPixel=true ;;
       no) customRegionScanPixel=false ;;
    esac],
   [customRegionScanPixel=false])
AM_CONDITIONAL([CUSTOM_REGIONSCANPIXEL], [test x$customRegionScanPixel = xtrue])
if test x$customRegionScanPixel = xtrue; then
   AC_DEFINE([CUSTOM_REGIONSCANPIXEL], [1], [Override customRegionScanPixel function])
fi

AC_CONFIG_FILES([libdmtx.pc])

AC_OUTPUT
