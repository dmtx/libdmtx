check_PROGRAMS = gltest

AM_CPPFLAGS=-I../..

gltest_SOURCES = gltest.c callback.c display.c image.c dmtx.c

EXTRA_gltest_SOURCES = callback.h display.h gltest.h image.h

if TARGET_MACOSX
gltest_LDFLAGS = -lm -lpng -framework OpenGL -lSDL -lSDLmain -framework Cocoa -lpthread
else
gltest_LDFLAGS = -lm -lpng -lGL -lGLU -lSDL -lpthread
endif
