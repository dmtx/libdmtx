#!/bin/sh

FILE="$1"

LINE=$(grep -n " $" $FILE)
if [[ $? -eq 0 ]]; then
   echo -e "Trailing whitespace found in $FILE on line(s):\n$LINE"
   exit 1
fi

exit 0
