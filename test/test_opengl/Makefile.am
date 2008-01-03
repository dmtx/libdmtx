check_PROGRAMS = gltest gltest.debug

AM_CPPFLAGS=-I/usr/include/SDL

gltest_SOURCES = gltest.c callback.c display.c image.c gltest.h callback.h display.h image.h
gltest_LDFLAGS = -lm -lpng -lGL -lGLU -lSDL -lpthread

gltest_debug_SOURCES = gltest.c callback.c display.c image.c gltest.h callback.h display.h image.h
gltest_debug_LDFLAGS = -lm -lpng -lGL -lGLU -lSDL -lpthread -static

LDADD = ../../libdmtx.la
