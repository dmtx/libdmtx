#!/bin/sh

FILES=$(find . -type f -name "*.[ch]" -o -name "*.sh")

for file in $FILES; do

   TODO_COUNT=$(grep -in -e "XXX" -e "TODO" -e "FIXME" $file | wc -l)
   if [[ "$TODO_COUNT" -gt 0 ]]; then
      printf "%4d TODO(s) remain in $file\n" $TODO_COUNT
   fi

done

exit 0
