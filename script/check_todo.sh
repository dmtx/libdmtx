#!/bin/sh

FILE="$1"

COUNT=$(grep -i -e "XXX" -e "TODO" -e "FIXME" $FILE | wc -l)
if [[ "$COUNT" -gt 0 ]]; then
   printf "%4d TODO(s) remain in $FILE\n" $COUNT
   exit 1
fi

exit 0
