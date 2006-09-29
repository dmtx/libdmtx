SHELL=/bin/sh
CC=gcc
INCLUDE=-I../..
LIBS=-lm
CFLAGS=-Wall -ggdb -pg

static: dmtxwrite.c dmtx.o
	$(CC) $(INCLUDE) $(CFLAGS) dmtxwrite.c -c
	$(CC) $(CFLAGS) $(LIBS) dmtx.o dmtxwrite.o -o dmtxwrite

dmtx.o:
	make -C ../.. static
	mv ../../dmtx.o .

dynamic: dmtxwrite.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) -L../.. -ldmtx dmtxwrite.c -o dmtxwrite

clean:
	rm -f *.o dmtxwrite gmon.out output.pnm

.PHONY: static dynamic clean
