#!/bin/sh

splint -linelen 999 -Disgreater -Disless dmtx.c

exit $?
