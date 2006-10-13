#!/bin/sh

LIBDMTX="$1"
if [[ -z "$LIBDMTX" ]]; then
   echo "Missing LIBDMTX paramter"
   exit 1
fi

FILES=$(find "$LIBDMTX" -type f -name "*.[ch]" -o -name "*.sh")

for file in $FILES; do

   TODO_COUNT=$(grep -in -e "XXX" -e "TODO" -e "FIXME" $file | wc -l)
   if [[ "$TODO_COUNT" -gt 0 ]]; then
      printf "%4d TODO(s) remain in $file\n" $TODO_COUNT
   fi

done

if [[ "$TODO_COUNT" -gt 0 ]]; then
#  echo "Problems found by \"$(basename $0)\"."
   exit 2
fi

exit 0
