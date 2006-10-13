#!/bin/sh

TOTAL_COUNT=0
FILES=$(find . -type f -name "*.[ch]" -o -name "*.sh")

for file in $FILES; do

   LINE=$(grep -n "\*\{10\}" $file)
   if [[ $? -eq 0 ]]; then
      echo -e "Bad comment style found in $file on line(s):\n$LINE"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

   LINE=$(sed -n '2,$ p' $file | grep -n "^\/\*\$")
   if [[ $? -eq 0 ]]; then
      echo -e "Bad comment style found in $file on line(s):\n$LINE"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\".  Aborting."
   exit 1
fi

exit 0
