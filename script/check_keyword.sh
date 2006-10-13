#!/bin/sh

TOTAL_COUNT=0
FILES=$(find . -type f -name "*.[ch]")

TEST="^\/\* \$\Id: .* \\$ \*\/\$"

for file in $FILES; do

   grep --silent "$TEST" $file
   if [[ $? -ne 0 ]]; then
      echo "Missing revision control keyword in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\".  Aborting."
   exit 1
fi

exit 0
