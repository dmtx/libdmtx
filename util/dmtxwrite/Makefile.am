bin_PROGRAMS = dmtxwrite
noinst_PROGRAMS = dmtxwrite.debug

dmtxwrite_SOURCES = dmtxwrite.c dmtxwrite.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxwrite_CFLAGS = $(PNG_CFLAGS)
dmtxwrite_LDFLAGS = -lm $(PNG_LIBS)

dmtxwrite_debug_SOURCES = dmtxwrite.c dmtxwrite.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxwrite_debug_CFLAGS = $(PNG_CFLAGS)
dmtxwrite_debug_LDFLAGS = -lm $(PNG_LIBS) -static

EXTRA_dmtxwrite_SOURCES = ../common/getopt1.c ../common/getopt.c \
	../common/getopt.h ../common/getopt_int.h ../common/gettext.h

LDADD = ../../libdmtx.la
