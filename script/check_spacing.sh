#!/bin/sh

set -u

FILE=$1

if [[ "$(basename $FILE)" = "dmtxfec.c" ]]; then
   exit 0
fi

PATTERN="XX_C_XXXX_XXXX_XXX_XX_X_X_"
COPYRIGHT=0

for i in $(seq 1 25); do
   LINE_TYPE=$(echo $PATTERN | cut -c$i)
   LINE_NBR=$((i + COPYRIGHT))
   if [[ "$LINE_TYPE" = "C" ]]; then
      while true; do
         sed -n "$LINE_NBR p" $FILE | grep --silent "^Copyright"
         if [[ $? -eq 0 ]]; then
            COPYRIGHT=$((COPYRIGHT+1))
            LINE_NBR=$((i + COPYRIGHT))
         else
            COPYRIGHT=$((COPYRIGHT-1))
            break
         fi
      done
   elif [[ "$LINE_TYPE" = "X" ]]; then
      sed -n "$LINE_NBR p" $FILE | grep --silent "^..*$"
      if [[ $? -ne 0 ]]; then
         echo "Expected line $LINE_NBR to be non-empty in $FILE"
         exit 1
      fi
   else
      sed -n "$LINE_NBR p" $FILE | grep --silent "^$"
      if [[ $? -ne 0 ]]; then
         echo "Expected line $LINE_NBR to be empty in $FILE"
         exit 1
      fi
   fi
done

exit 0
