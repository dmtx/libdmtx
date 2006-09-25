INCLUDE=-I../..
LIBS=-lpng -ltiff -L../.. -ldmtx
CFLAGS=-Wall -ggdb

all: dmtxread.bin

clean:
	rm -f *.o dmtxread.bin gmon.out

dmtxread.bin: dmtxread.c
	gcc $(INCLUDE) $(CFLAGS) $(LIBS) dmtxread.c -o dmtxread.bin

.PHONY: all clean
