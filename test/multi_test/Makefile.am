AM_CPPFLAGS = -Wshadow -Wall -pedantic -ansi

check_PROGRAMS = multi_test

multi_test_SOURCES = multi_test.c dmtx.c

if TARGET_MACOSX
multi_test_LDFLAGS = -lm -lSDL -lSDL_image -lSDMmain -framework Cocoa -lpthread
else
multi_test_LDFLAGS = -lm -lSDL -lSDL_image -lpthread
endif
