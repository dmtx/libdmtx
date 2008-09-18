bin_PROGRAMS = dmtxread
noinst_PROGRAMS = dmtxread.debug

dmtxread_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_LDFLAGS = -lm -lpng

dmtxread_debug_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_debug_LDFLAGS = -lm -lpng -static

if HAVE_LIBJPEG
dmtxread_LDFLAGS += -ljpeg
dmtxread_debug_LDFLAGS += -ljpeg
endif

if HAVE_LIBTIFF
dmtxread_LDFLAGS += -ltiff
dmtxread_debug_LDFLAGS += -ltiff
endif

EXTRA_dmtxread_SOURCES = ../common/getopt1.c ../common/getopt.c \
        ../common/getopt.h ../common/getopt_int.h ../common/gettext.h

LDADD = ../../libdmtx.la
