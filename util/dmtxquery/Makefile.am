AM_CPPFLAGS = -Wshadow -Wall -pedantic

bin_PROGRAMS = dmtxquery
noinst_PROGRAMS = dmtxquery.debug

dmtxquery_SOURCES = dmtxquery.c dmtxquery.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxquery_LDFLAGS = -lm

dmtxquery_debug_SOURCES = dmtxquery.c dmtxquery.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxquery_debug_LDFLAGS = -lm -static

EXTRA_dmtxquery_SOURCES = ../common/getopt1.c ../common/getopt.c \
        ../common/getopt.h ../common/getopt_int.h ../common/gettext.h

LDADD = ../../libdmtx.la
