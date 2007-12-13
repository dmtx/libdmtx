#!/bin/sh

FILE="$1"

PATTERN="XX_X_XXXX_XXXX_XXX_XX_X_X_"

for i in $(seq 1 25); do
   LINE=$(echo $PATTERN | cut -c$i)
   if [[ "$LINE" = "X" ]]; then
      sed -n "$i p" $FILE | grep --silent "^..*$"
      if [[ $? -ne 0 ]]; then
         echo "Expected line $i to be non-empty in $FILE"
         exit 1
      fi
   else
      sed -n "$i p" $FILE | grep --silent "^$"
      if [[ $? -ne 0 ]]; then
         echo "Expected line $i to be empty in $FILE"
         exit 1
      fi
   fi
done

exit 0
