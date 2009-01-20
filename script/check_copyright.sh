#!/bin/sh

FILE="$1"

if [[ $(cat $FILE | wc -l) -le 10 ]]; then
   echo "File \"$FILE\" considered trivial for copyright purposes (<=10 lines)"
   exit 0
fi

# Every nontrivial source file must include a copyright line
COPYRIGHT=$(grep "Copyright (C) " $FILE)
if [[ $? -ne 0 ]]; then
   echo "Missing copyright text in $FILE"
   exit 1
fi

# Copyright line must contain the current year
echo "$COPYRIGHT" | grep --silent $(date '+%Y')
if [[ $? -ne 0 ]]; then
   echo "Missing or incorrect copyright year in $FILE"
   exit 2
fi

exit 0
