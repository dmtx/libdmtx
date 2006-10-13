#!/bin/sh

LIBDMTX="$1"
if [[ -z "$LIBDMTX" ]]; then
   echo "Missing LIBDMTX paramter"
   exit 1
fi

TOTAL_COUNT=0
FILES=$(find "$LIBDMTX" -type f -name "*.[ch]")

for file in $FILES; do

   # Every source file must include a copyright line
   COPYRIGHT=$(grep Copyright $file)
   if [[ $? -ne 0 ]]; then
      echo "Missing copyright text in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
      continue;
   fi

   # Copyright line must contain the current year
   echo "$COPYRIGHT" | grep --silent $(date '+%Y')
   if [[ $? -ne 0 ]]; then
      echo "Missing or incorrect copyright year in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
      continue;
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\"."
   exit 2
fi

exit 0
