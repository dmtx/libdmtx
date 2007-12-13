#!/bin/sh

FILE="$1"

# Every source file must include a copyright line
COPYRIGHT=$(grep "Copyright (c) " $FILE)
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
