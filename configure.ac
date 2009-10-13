AC_INIT([libdmtx], [0.7.3], [mike@dragonflylogic.com])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AC_CONFIG_MACRO_DIR([m4])
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

AC_PROG_CC
AC_PROG_LIBTOOL
AM_PROG_CC_C_O

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

AC_ARG_ENABLE(
   [dmtxquery],
   AS_HELP_STRING([--disable-dmtxquery], [do not build the dmtxquery command line utility]),
   [dmtxquery="$enableval"],
   [dmtxquery="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXQUERY], [test x$dmtxquery = xyes])

AC_ARG_ENABLE(
   [dmtxread],
   AS_HELP_STRING([--disable-dmtxread], [do not build the dmtxread command line utility]),
   [dmtxread="$enableval"],
   [dmtxread="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXREAD], [test x$dmtxread = xyes])

AC_ARG_ENABLE(
   [dmtxwrite],
   AS_HELP_STRING([--disable-dmtxwrite], [do not build the dmtxwrite command line utility]),
   [dmtxwrite="$enableval"],
   [dmtxwrite="yes"]
)
AM_CONDITIONAL([ENABLE_DMTXWRITE], [test x$dmtxwrite = xyes])

if test x$dmtxread = xyes \
      -o x$dmtxwrite = xyes \
      -o x$dmtxquery = xyes; then
   AC_CHECK_HEADERS([sysexits.h])
   AC_CHECK_HEADERS([getopt.h])
   AC_CHECK_FUNCS([getopt_long])
fi

if test x$dmtxread = xyes \
      -o x$dmtxwrite = xyes; then
   ifdef([PKG_CHECK_MODULES],
         [PKG_CHECK_MODULES(
            MAGICK,
            Wand >= 6.2.4,
            true,
            AC_MSG_ERROR([[dmtxread/dmtxwrite requires Wand >= 6.2.4]]))],
         [AC_MSG_ERROR([Building dmtxread/dmtxwrite requires a working autoconf/pkg-config])]
   )
   AC_SUBST(MAGICK_CFLAGS)
   AC_SUBST(MAGICK_LIBS)
fi

if test x$dmtxquery = xyes; then
   AC_CONFIG_FILES([util/dmtxquery/Makefile])
fi

if test x$dmtxread = xyes; then
   AC_CONFIG_FILES([util/dmtxread/Makefile])
fi

if test x$dmtxwrite = xyes; then
   AC_CONFIG_FILES([util/dmtxwrite/Makefile])
fi

# Create options to override certain functions

# --enable-customEncodeDataMatrix
AC_ARG_ENABLE(
   [customEncodeDataMatrix],
   [],
   [customEncodeDataMatrix="$enableval"],
   [customEncodeDataMatrix="no"]
)
AM_CONDITIONAL([CUSTOM_ENCODEDATAMATRIX], [test x$customEncodeDataMatrix = xyes])
if test x$customEncodeDataMatrix = xyes; then
   AC_DEFINE([CUSTOM_ENCODEDATAMATRIX], [1], [Override dmtxEncodeDataMatrix function])
fi

# --enable-customEncodeDataMosaic
AC_ARG_ENABLE(
   [customEncodeDataMosaic],
   [],
   [customEncodeDataMosaic="$enableval"],
   [customEncodeDataMosaic="no"]
)
AM_CONDITIONAL([CUSTOM_ENCODEDATAMOSAIC], [test x$customEncodeDataMosaic = xyes])
if test x$customEncodeDataMosaic = xyes; then
   AC_DEFINE([CUSTOM_ENCODEDATAMOSAIC], [1], [Override customEncodeDataMosaic function])
fi

# --enable-customDecodeMatrixRegion
AC_ARG_ENABLE(
   [customDecodeMatrixRegion],
   [],
   [customDecodeMatrixRegion="$enableval"],
   [customDecodeMatrixRegion="no"]
)
AM_CONDITIONAL([CUSTOM_DECODEMATRIXREGION], [test x$customDecodeMatrixRegion = xyes])
if test x$customDecodeMatrixRegion = xyes; then
   AC_DEFINE([CUSTOM_DECODEMATRIXREGION], [1], [Override customDecodeMatrixRegion function])
fi

# --enable-customDecodeMosaicRegion
AC_ARG_ENABLE(
   [customDecodeMosaicRegion],
   [],
   [customDecodeMosaicRegion="$enableval"],
   [customDecodeMosaicRegion="no"]
)
AM_CONDITIONAL([CUSTOM_DECODEMOSAICREGION], [test x$customDecodeMosaicRegion = xyes])
if test x$customDecodeMosaicRegion = xyes; then
   AC_DEFINE([CUSTOM_DECODEMOSAICREGION], [1], [Override customDecodeMosaicRegion function])
fi

# --enable-customRegionFindNext
AC_ARG_ENABLE(
   [customRegionFindNext],
   [],
   [customRegionFindNext="$enableval"],
   [customRegionFindNext="no"]
)
AM_CONDITIONAL([CUSTOM_REGIONFINDNEXT], [test x$customRegionFindNext = xyes])
if test x$customRegionFindNext = xyes; then
   AC_DEFINE([CUSTOM_REGIONFINDNEXT], [1], [Override customRegionFindNext function])
fi

# --enable-customRegionScanPixel
AC_ARG_ENABLE(
   [customRegionScanPixel],
   [],
   [customRegionScanPixel="$enableval"],
   [customRegionScanPixel="no"]
)
AM_CONDITIONAL([CUSTOM_REGIONSCANPIXEL], [test x$customRegionScanPixel = xyes])
if test x$customRegionScanPixel = xyes; then
   AC_DEFINE([CUSTOM_REGIONSCANPIXEL], [1], [Override customRegionScanPixel function])
fi

AC_ARG_ENABLE(
   [cocoa],
   AS_HELP_STRING([--enable-cocoa], [enable Cocoa bindings]),
   [enable_cocoa="$enableval"],
   [enable_cocoa="no"]
)
AM_CONDITIONAL(ENABLE_COCOA, [test x$enable_cocoa = xyes])

if test x$enable_cocoa = xyes; then
   AC_WARN([Building the Cocoa wrapper though the libdmtx build system is not yet supported])
fi

AC_ARG_ENABLE(
   [java],
   AS_HELP_STRING([--enable-java], [enable Java bindings]),
   [enable_java="$enableval"],
   [enable_java="no"]
)
AM_CONDITIONAL(ENABLE_JAVA, [test x$enable_java = xyes])

if test x$enable_java = xyes; then
   AC_WARN([Building the Java wrapper though the libdmtx build system is not yet supported])
fi

AC_ARG_ENABLE(
   [net],
   AS_HELP_STRING([--enable-net], [enable .NET bindings]),
   [enable_net="$enableval"],
   [enable_net="no"]
)
AM_CONDITIONAL(ENABLE_NET, [test x$enable_net = xyes])

if test x$enable_net = xyes; then
   AC_WARN([Building the .NET wrapper though the libdmtx build system is not yet supported])
fi

AC_ARG_ENABLE(
   [php],
   AS_HELP_STRING([--enable-php], [enable PHP bindings]),
   [enable_php="$enableval"],
   [enable_php="no"]
)
AM_CONDITIONAL([ENABLE_PHP], [test x$enable_php = xyes])

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

AC_ARG_ENABLE(
   [python],
   AS_HELP_STRING([--enable-python], [enable Python bindings]),
   [enable_python="$enableval"],
   [enable_python="no"]
)
AM_CONDITIONAL(ENABLE_PYTHON, [test x$enable_python = xyes])

if test x$enable_python = xyes; then
   AM_PATH_PYTHON
   AC_WARN([Building the Python wrapper will break make uninstall])
fi

AC_ARG_ENABLE(
   [ruby],
   AS_HELP_STRING([--enable-ruby], [enable Ruby bindings]),
   [enable_ruby="$enableval"],
   [enable_ruby="no"]
)
AM_CONDITIONAL(ENABLE_RUBY, [test x$enable_ruby = xyes])

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

AC_ARG_ENABLE(
   [vala],
   AS_HELP_STRING([--enable-vala], [enable Vala bindings]),
   [enable_vala="$enableval"],
   [enable_vala="no"]
)
AM_CONDITIONAL(ENABLE_VALA, [test x$enable_vala = xyes])

if test x$enable_vala = xyes; then
   PKG_PROG_PKG_CONFIG

   PKG_CHECK_EXISTS(
      [vala-1.0],
      [VALA_VAPIDIR=`$PKG_CONFIG --variable=vapidir vala-1.0`],
      [
         AC_WARN([Can't find vala development files, guessing where to install vala bindings...])
         VALA_VAPIDIR='${prefix}/share/vala/vapi'
      ]
   )

   AC_SUBST(VALA_VAPIDIR)
   AC_CONFIG_FILES([wrapper/vala/Makefile])
fi

AC_CONFIG_FILES([libdmtx.pc])

AC_OUTPUT
