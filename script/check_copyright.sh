#!/bin/sh

FILE="$1"

# Every nontrivial source file must include a copyright line
COPYRIGHT=$(grep "Copyright 2[[:digit:]]\{3\}" $FILE)
if [[ $? -ne 0 ]]; then
   echo "Missing copyright text in $FILE"
   exit 1
fi

exit 0
