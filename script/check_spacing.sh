#!/bin/sh

LIBDMTX="$1"
if [[ -z "$LIBDMTX" ]]; then
   echo "Missing LIBDMTX paramter"
   exit 1
fi

BLANK_LINES="4 9 14 18 21 23"
NON_BLANK_LINES="1 2 3 5 6 7 8 10 11 12 13 15 16 17 19 20 22 24"

TOTAL_COUNT=0
FILES=$(find "$LIBDMTX" -type f -name "*.[ch]")

for file in $FILES; do

   FILE_COUNT=0
   for line in $BLANK_LINES; do
      sed -n "$line p" $file | grep --silent "^$"
      FILE_COUNT=$(( FILE_COUNT + $? ))
   done

   for line in $NON_BLANK_LINES; do
      sed -n "$line p" $file | grep --silent "^..*$"
      FILE_COUNT=$(( FILE_COUNT + $? ))
   done

   if [[ "$FILE_COUNT" -gt 0 ]]; then
      echo "Found blank lines out of place in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\"."
   exit 2
fi

exit 0
