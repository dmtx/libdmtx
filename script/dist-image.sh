#!/bin/sh

VERSION=$(grep AC_INIT configure.ac | awk '{print $2}' | sed 's/[^0-9\.]//g')

SRC_DIST=libdmtx-$VERSION
IMG_DIST=libdmtx-images-$VERSION

IMG_DIRS=" \
$SRC_DIST/test/images_core \
$SRC_DIST/test/images_omar \
$SRC_DIST/test/images_sportcam"

tar --exclude CVS -cvf $IMG_DIST.tar -C .. $IMG_DIRS

exit 0
