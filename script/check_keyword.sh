#!/bin/sh

TOTAL_COUNT=0
FILES=$(find . -type f -name "*.[ch]")

TEST="^\/\* \$\Id: .* \\$ \*\/\$"

for file in $FILES; do

   grep --silent "$TEST" $file
   if [[ $? -ne 0 ]]; then
      echo -e "\nMissing revision control keyword: $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo -e "\nProblems found by \"$(basename $0)\".  Aborting."
fi

exit $COUNT
