# Packaging commands (all run from libdmtx root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip

AUTOMAKE_OPTIONS = foreign

ACLOCAL_AMFLAGS = -I m4
AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

lib_LTLIBRARIES = libdmtx.la
libdmtx_la_SOURCES = dmtx.c
libdmtx_la_CFLAGS = -Wall -pedantic

EXTRA_libdmtx_la_SOURCES = dmtxencode.c dmtxencodestream.c dmtxencodescheme.c \
	dmtxencodeoptimize.c dmtxencodeascii.c dmtxencodec40textx12.c \
	dmtxencodeedifact.c dmtxencodebase256.c dmtxdecode.c dmtxdecodescheme.c \
	dmtxmessage.c dmtxregion.c dmtxsymbol.c dmtxplacemod.c dmtxreedsol.c \
	dmtxscangrid.c dmtximage.c dmtxbytelist.c dmtxtime.c dmtxvector2.c \
	dmtxmatrix3.c dmtxstatic.h

include_HEADERS = dmtx.h

SUBDIRS = . test

dist_man_MANS = man/libdmtx.3

EXTRA_DIST = KNOWNBUG \
	LICENSE \
	README.cygwin \
	README.freebsd \
	README.linux \
	README.mingw \
	README.osx \
	README.unix \
	m4 \
	script/check_all.sh \
	script/check_comments.sh \
	script/check_copyright.sh \
	script/check_headers.pl \
	script/check_license.sh \
	script/check_spacing.sh \
	script/check_splint.sh \
	script/check_todo.sh \
	script/check_whitespace.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdmtx.pc
