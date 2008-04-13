check_PROGRAMS = gltest

AM_CPPFLAGS=-I/usr/include/SDL -I../..

gltest_SOURCES = gltest.c callback.c display.c image.c dmtx.c \
        ../../libfec/decode_rs_char.c ../../libfec/encode_rs_char.c \
        ../../libfec/init_rs_char.c

EXTRA_gltest_SOURCES = callback.h display.h gltest.h image.h

gltest_LDFLAGS = -lm -lpng -lGL -lGLU -lSDL -lpthread
