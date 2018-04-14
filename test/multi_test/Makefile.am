AM_CPPFLAGS = -Wshadow -Wall -pedantic -std=c99

check_PROGRAMS = multi_test

multi_test_SOURCES = dmtx.c dmtxaccel.c dmtxdecode2.c dmtxhough.c \
	dmtxregion2.c dmtxsobel.c dmtxvaluegrid.c kiss_fft.c kiss_fftr.c \
	multi_test.c multi_test.h visualize.c

if TARGET_MACOSX
multi_test_LDFLAGS = -lm -lSDL -lSDL_image -lSDL_gfx -lSDMmain -framework Cocoa -lpthread
else
multi_test_LDFLAGS = -lm -lSDL -lSDL_image -lSDL_gfx -lpthread -L/usr/local/lib -L/usr/lib/mingw
endif
