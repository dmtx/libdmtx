#!/bin/sh

LIBDMTX="$1"
if [[ -z "$LIBDMTX" ]]; then
   echo "Missing LIBDMTX paramter"
   exit 1
fi

TOTAL_COUNT=0
FILES=$(find "$LIBDMTX" -type f -name "*.[ch]")

TEST="^\/\* \$\Id: .* \\$ \*\/\$"

for file in $FILES; do

   grep --silent "$TEST" $file
   if [[ $? -ne 0 ]]; then
      echo "Missing revision control keyword in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\"."
   exit 2
fi

exit 0
