#!/bin/sh

FILE="$1"

TEST="^\/\* \$\Id: .* \\$ \*\/\$"

grep --silent "$TEST" $FILE
if [[ $? -ne 0 ]]; then
   echo "Missing revision control keyword in $FILE"
   exit 1
fi

exit 0
