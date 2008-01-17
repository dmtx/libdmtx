# Packaging commands (all run from libdmtx root):
# $ make distclean
# $ make dist-bzip2
# $ make dist-gzip
# $ script/dist-image.sh

lib_LTLIBRARIES = libdmtx.la
libdmtx_la_SOURCES = dmtx.c dmtxstatic.h
EXTRA_libdmtx_la_SOURCES = dmtxcallback.c dmtxcolor3.c dmtxdecode.c \
	dmtxencode.c dmtximage.c dmtxmatrix3.c dmtxplacemod.c \
	dmtxreedsol.c dmtxregion.c dmtxsymbol.c dmtxvector2.c
include_HEADERS = dmtx.h
libdmtx_la_CPPFLAGS = -DCALLBACKS
libdmtx_la_CFLAGS = -Wall -pedantic

SUBDIRS = . util test

dist_man_MANS = man/libdmtx.3 man/dmtxread.1 man/dmtxwrite.1

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
	bindings/python/* \
	bindings/php/*
