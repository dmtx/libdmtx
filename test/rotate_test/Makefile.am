check_PROGRAMS = rotate_test

AM_CPPFLAGS=-I../..

rotate_test_SOURCES = rotate_test.c callback.c display.c image.c dmtx.c

EXTRA_rotate_test_SOURCES = callback.h display.h rotate_test.h image.h

if TARGET_MACOSX
rotate_test_LDFLAGS = -lm -lpng -framework OpenGL -lSDL -lSDLmain -framework Cocoa -lpthread
else
rotate_test_LDFLAGS = -lm -lpng -lGL -lGLU -lSDL -lpthread
endif
