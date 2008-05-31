bin_PROGRAMS = dmtxread
noinst_PROGRAMS = dmtxread.debug

dmtxread_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_LDFLAGS = -lm -lpng -ltiff

dmtxread_debug_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_debug_LDFLAGS = -lm -lpng -ltiff -static

EXTRA_dmtxread_SOURCES = ../common/getopt1.c ../common/getopt.c \
        ../common/getopt.h ../common/getopt_int.h ../common/gettext.h

LDADD = ../../libdmtx.la
