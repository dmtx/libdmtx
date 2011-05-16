#!/bin/sh

FILE="$1"

TEST1="^ \* libdmtx - Data Matrix Encoding/Decoding Library\$"
TEST2="^ \* See LICENSE file in the main project directory for full\$"
TEST3="^ \* terms of use and distribution.\$"
TEST4="^ \* Contact: Mike Laughton <mike@dragonflylogic.com>\$"

COUNT=0

grep --silent "$TEST1" $FILE
COUNT=$(( COUNT + $? ))

grep --silent "$TEST2" $FILE
COUNT=$(( COUNT + $? ))

grep --silent "$TEST3" $FILE
COUNT=$(( COUNT + $? ))

grep --silent "$TEST4" $FILE
COUNT=$(( COUNT + $? ))

if [[ "$COUNT" -gt 0 ]]; then
   echo "Missing license text in $FILE"
   exit 1
fi

exit 0
