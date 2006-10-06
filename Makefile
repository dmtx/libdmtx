SHELL=/bin/sh
CC=gcc
CFLAGS=-Wall -ggdb -pg
LIBFLAGS=-fPIC
SOURCES=dmtx.h dmtxstatic.h dmtx.c dmtxregion.c dmtxdecode.c dmtxencode.c \
      dmtxplacemod.c dmtxgalois.c dmtxreedsol.c dmtxvector2.c dmtxmatrix3.c \
      dmtxcolor3.c dmtximage.c dmtxcallback.c
CHCON=/usr/bin/chcon

libdmtx.so: libdmtx.so.0.3.0
	ln -sf libdmtx.so.0.3.0 libdmtx.so.1
	ln -sf libdmtx.so.1 libdmtx.so
	if [ -x $(CHCON) ]; then $(CHCON) -t textrel_shlib_t libdmtx.so; fi
	# I haven't found a good solution for the chcon problem yet.  It will
	# go away when we start installing the library to a valid lib dir.

libdmtx.so.0.3.0: $(SOURCES)
	$(CC) $(CFLAGS) -shared -Wl,-soname,libdmtx.so.1 -Wl,-export-dynamic \
		-o libdmtx.so.0.3.0 dmtx.c -lc -lm

all: test util

static: $(SOURCES)
	$(CC) $(CFLAGS) dmtx.c -c

test: libdmtx.so
	make -C test/gltest
	make -C test/simpletest

util: libdmtx.so
	make -C util/dmtxread
	make -C util/dmtxwrite

tarball:
	make clean
	tar -cvf ../libdmtx.tar -C .. libdmtx
	bzip2 -f ../libdmtx.tar

clean:
	rm -f *.o *.d lib*.so*
	make -C test/gltest clean
	make -C test/simpletest clean
	make -C util/dmtxread clean
	make -C util/dmtxwrite clean

.PHONY: all static test util tarball clean
