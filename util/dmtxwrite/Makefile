INCLUDE=-I../..
LIBS=-L../.. -ldmtx -lpng
CFLAGS=-Wall -ggdb

all: dmtxwrite.bin

clean:
	rm -f *.o dmtxwrite.bin gmon.out *.pnm

dmtxwrite.bin: dmtxwrite.c
	gcc $(INCLUDE) $(CFLAGS) $(LIBS) dmtxwrite.c -o dmtxwrite.bin

.PHONY: all clean
