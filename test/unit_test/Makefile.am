AM_CPPFLAGS = -Wshadow -Wall -pedantic -std=c99

check_PROGRAMS = unit_test

unit_test_SOURCES = unit_test.c ../../util/common/dmtxutil.c ../../util/common/dmtxutil.h
unit_test_LDFLAGS = -lm

LDADD = ../../libdmtx.la
