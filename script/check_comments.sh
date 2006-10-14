#!/bin/sh

FILE="$1"

LINE=$(grep -n "\*\{10\}" $FILE)
if [[ $? -eq 0 ]]; then
   echo -e "Bad comment style found in $FILE on line(s):\n$LINE"
   exit 1
fi

LINE=$(sed -n '2,$ p' $FILE | grep -n "^\/\*\$")
if [[ $? -eq 0 ]]; then
   echo -e "Bad comment style found in $FILE on line(s):\n$LINE"
   exit 2
fi

exit 0
