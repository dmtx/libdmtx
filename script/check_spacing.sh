#!/bin/sh

FILE="$1"

BLANK_LINES="4 9 14 18 21 23"
NON_BLANK_LINES="1 2 3 5 6 7 8 10 11 12 13 15 16 17 19 20 22 24"

COUNT=0
for line in $BLANK_LINES; do
   sed -n "$line p" $FILE | grep --silent "^$"
   COUNT=$(( COUNT + $? ))
done

for line in $NON_BLANK_LINES; do
   sed -n "$line p" $FILE | grep --silent "^..*$"
   COUNT=$(( COUNT + $? ))
done

if [[ "$COUNT" -gt 0 ]]; then
   echo "Found blank lines out of place in $FILE"
   exit 1
fi

exit 0
