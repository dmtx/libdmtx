CC=gcc
CFLAGS=-Wall -g -pg
LIBFLAGS=-fPIC
SOURCES=dmtx.h dmtxstatic.h dmtx.c dmtxregion.c dmtxdecode.c dmtxencode.c \
      dmtxplacemod.c dmtxgalois.c dmtxreedsol.c dmtxvector2.c dmtxvector3.c \
      dmtxmatrix3.c dmtxcolor.c dmtximage.c

libdmtx.so: libdmtx.so.0.3.0
	ln -sf libdmtx.so.0.3.0 libdmtx.so.1
	ln -sf libdmtx.so.1 libdmtx.so
	chcon -t textrel_shlib_t libdmtx.so

libdmtx.so.0.3.0: $(SOURCES)
	$(CC) $(CFLAGS) -shared -Wl,-soname,libdmtx.so.1 -Wl,-export-dynamic \
		-o libdmtx.so.0.3.0 dmtx.c -lc -lm -lpng

all: test util

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

.PHONY: all test util tarball clean
