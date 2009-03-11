IMAGE_CLASS=org/libdmtx/DMTXImage.class
IMAGE_JAVA=org/libdmtx/DMTXImage.java

TAG_CLASS=org/libdmtx/DMTXTag.class
TAG_JAVA=org/libdmtx/DMTXTag.java

DMTX_JAR=dmtx.jar

NATIVE_C=native/org_libdmtx_DMTXImage.c
NATIVE_H=native/org_libdmtx_DMTXImage.h
NATIVE_SO=native/libdmtx.so

LIBDMTX_LA=../../libdmtx_la-dmtx.o
CFLAGS=-shared -fpic
INCLUDE=-I ../.. \
	-I /usr/lib/jvm/java-1.6.0-openjdk/include \
	-I /usr/lib/jvm/java-1.6.0-openjdk/include/linux

GENERATED=$(IMAGE_CLASS) $(TAG_CLASS) $(NATIVE_H) $(NATIVE_SO) $(DMTX_JAR)

all: $(GENERATED)

check: $(GENERATED)
	javac GUIExample.java
	javac CLIExample.java
	javac MakeTags.java

clean:
	rm -f $(GENERATED) GUIExample.class CLIExample.class

$(LIBDMTX_LA):
	make libdmtx.la -C ../..

$(NATIVE_H):
	javah -jni -d native org.libdmtx.DMTXImage

$(NATIVE_SO): $(NATIVE_C) $(NATIVE_H) $(LIBDMTX_LA)
	gcc $(NATIVE_C) $(CFLAGS) -o $(NATIVE_SO) $(INCLUDE) $(LIBDMTX_LA)

$(IMAGE_CLASS) $(TAG_CLASS): $(IMAGE_JAVA) $(TAG_JAVA)
	javac org/libdmtx/DMTXImage.java

$(DMTX_JAR) : $(IMAGE_CLASS) $(TAG_CLASS)
	jar cf $(DMTX_JAR) $(IMAGE_CLASS) $(TAG_CLASS)

.PHONY: all check clean
