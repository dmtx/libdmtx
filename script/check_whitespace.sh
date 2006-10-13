#!/bin/sh

TOTAL_COUNT=0
FILES=$(find . -type f -name "*.[ch]" -o -name "*.sh")

for file in $FILES; do

   LINE=$(grep -n " $" $file)
   if [[ $? -eq 0 ]]; then
      echo -e "Trailing whitespace found in $file on line(s):\n$LINE"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\".  Aborting."
   exit 1
fi

exit 0
