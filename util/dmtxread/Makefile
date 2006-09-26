SHELL=/bin/sh
CC=gcc
INCLUDE=-I../..
LIBS=-lpng -ltiff -L../.. -ldmtx
CFLAGS=-Wall -ggdb -pg

all: dmtxread.bin

static: dmtxread.c dmtx.o
	$(CC) $(INCLUDE) $(CFLAGS) dmtxread.c -c
	$(CC) -pg -lm -lpng dmtx.o dmtxread.o -o dmtxread

clean:
	rm -f *.o dmtxread.bin gmon.out

dmtx.o:
	make -C ../.. static
	mv ../../dmtx.o .

dmtxread.bin: dmtxread.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) dmtxread.c -o dmtxread.bin

.PHONY: all clean static
