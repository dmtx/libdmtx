#!/bin/sh

FILE="$1"

TEST1="^modify it under the terms of the GNU Lesser General Public\$"
TEST2="^MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE\.  See the GNU\$"
TEST3="^You should have received a copy of the GNU Lesser General Public\$"

COUNT=0

grep --silent "$TEST1" $FILE
COUNT=$(( COUNT + $? ))

grep --silent "$TEST2" $FILE
COUNT=$(( COUNT + $? ))

grep --silent "$TEST3" $FILE
COUNT=$(( COUNT + $? ))

if [[ "$COUNT" -gt 0 ]]; then
   echo "Missing license text in $FILE"
   exit 1
fi

exit 0
