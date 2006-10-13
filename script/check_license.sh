#!/bin/sh

TOTAL_COUNT=0
FILES=$(find . -type f -name "*.[ch]")

TEST1="^modify it under the terms of the GNU Lesser General Public\$"
TEST2="^MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\.  See the GNU\$"
TEST3="^You should have received a copy of the GNU Lesser General Public\$"

for file in $FILES; do

   FILE_COUNT=0

   grep --silent "$TEST1" $file
   FILE_COUNT=$(( FILE_COUNT + $? ))

   grep --silent "$TEST2" $file
   FILE_COUNT=$(( FILE_COUNT + $? ))

   grep --silent "$TEST3" $file
   FILE_COUNT=$(( FILE_COUNT + $? ))

   if [[ "$FILE_COUNT" -ne 0 ]]; then
      echo "Missing license text in $file"
      TOTAL_COUNT=$(( TOTAL_COUNT + 1 ))
   fi

done

if [[ "$TOTAL_COUNT" -gt 0 ]]; then
   echo "Problems found by \"$(basename $0)\".  Aborting."
   exit 1
fi

exit 0
