#!/bin/sh

#splint -linelen 999 -Disgreater -Disless dmtx.c
splint -linelen 999 dmtx.c

exit $?
