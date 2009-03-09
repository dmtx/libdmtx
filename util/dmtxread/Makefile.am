AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

bin_PROGRAMS = dmtxread
noinst_PROGRAMS = dmtxread.debug

dmtxread_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_CFLAGS = $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxread_LDFLAGS = -lm $(MAGICK_LIBS)

dmtxread_debug_SOURCES = dmtxread.c dmtxread.h ../common/dmtxutil.c ../common/dmtxutil.h
dmtxread_debug_CFLAGS = $(MAGICK_CFLAGS) -D_MAGICK_CONFIG_H
dmtxread_debug_LDFLAGS = -lm $(MAGICK_LIBS) -static

EXTRA_dmtxread_SOURCES = ../common/getopt1.c ../common/getopt.c \
        ../common/getopt.h ../common/getopt_int.h ../common/gettext.h

LDADD = ../../libdmtx.la
