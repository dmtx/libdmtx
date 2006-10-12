SHELL=/bin/sh
CC=gcc
INCLUDE=-I../..
LIBS=-lpng -ltiff -lm
#CFLAGS=-Wall -ggdb -pg
CFLAGS=-Wall -ggdb

static: dmtxread.c dmtx.o
	$(CC) $(INCLUDE) $(CFLAGS) dmtxread.c -c
	$(CC) $(CFLAGS) $(LIBS) dmtx.o dmtxread.o -o dmtxread

dmtx.o:
	make -C ../.. static
	mv ../../dmtx.o .

dynamic: dmtxread.c
	$(CC) $(INCLUDE) $(CFLAGS) $(LIBS) -L../.. -ldmtx dmtxread.c -o dmtxread

clean:
	rm -f *.o dmtxread gmon.out

.PHONY: static dynamic clean
