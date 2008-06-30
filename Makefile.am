# Packaging commands (all run from libdmtx root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip
# $ script/dist-image.sh

lib_LTLIBRARIES = libdmtx.la
libdmtx_la_SOURCES = dmtx.c
EXTRA_libdmtx_la_SOURCES = dmtxtime.c dmtxscangrid.c dmtxmessage.c \
	dmtxregion.c dmtxdecode.c dmtxencode.c dmtxplacemod.c dmtxfec.c \
	dmtxreedsol.c dmtxsymbol.c dmtxvector2.c dmtxmatrix3.c dmtxcolor3.c \
	dmtximage.c

include_HEADERS = dmtx.h
libdmtx_la_CFLAGS = -Wall -pedantic

SUBDIRS = . util test

dist_man_MANS = man/libdmtx.3 man/dmtxread.1 man/dmtxwrite.1 man/dmtxquery.1

EXTRA_DIST = KNOWNBUG \
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
	test/images_opengl/* \
	wrappers/cocoa/* \
	wrappers/python/* \
	wrappers/php/*
