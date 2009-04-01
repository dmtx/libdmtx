# Packaging commands (all run from libdmtx root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip
# $ script/dist-image.sh

AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

lib_LTLIBRARIES = libdmtx.la
libdmtx_la_SOURCES = dmtx.c
libdmtx_la_LIBADD = $(LIBOBJS)
libdmtx_la_CFLAGS = -Wall -pedantic
EXTRA_libdmtx_la_SOURCES = dmtxdecode.c dmtxencode.c dmtxfec.c dmtximage.c \
	dmtxmatrix3.c dmtxmessage.c dmtxplacemod.c dmtxreedsol.c dmtxregion.c \
	dmtxscangrid.c dmtxstatic.h dmtxsymbol.c dmtxtime.c dmtxvector2.c

include_HEADERS = dmtx.h

SUBDIRS = . util test

dist_man_MANS = man/libdmtx.3 man/dmtxread.1 man/dmtxwrite.1 man/dmtxquery.1

EXTRA_DIST = KNOWNBUG \
	README.cygwin \
	README.freebsd \
	README.linux \
	README.mingw \
	README.osx \
	README.unix \
	README.visualc \
	project \
	script/check_all.sh \
	script/check_comments.sh \
	script/check_copyright.sh \
	script/check_headers.pl \
	script/check_keyword.sh \
	script/check_license.sh \
	script/check_spacing.sh \
	script/check_todo.sh \
	script/check_whitespace.sh \
	script/dist-image.sh \
	test/rotate_test/images/test_image??.png \
	wrapper/cocoa/* \
	wrapper/java/* \
	wrapper/net/* \
	wrapper/php/* \
	wrapper/python/* \
	wrapper/ruby/*

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdmtx.pc
