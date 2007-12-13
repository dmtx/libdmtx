bin_PROGRAMS = dmtxread
noinst_PROGRAMS = dmtxread.debug

dmtxread_SOURCES = dmtxread.c dmtxread.h
dmtxread_LDFLAGS = -lm -lpng -ltiff

dmtxread_debug_SOURCES = dmtxread.c
dmtxread_debug_LDFLAGS = -lm -lpng -ltiff -static

LDADD = ../../libdmtx.la
