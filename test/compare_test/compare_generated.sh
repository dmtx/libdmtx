#!/bin/sh

#SCHEMES="b f a c t x e 8"
SCHEMES="b a c t x e 8"
DMTXWRITE="../../util/dmtxwrite/dmtxwrite"
DMTXREAD="../../util/dmtxread/dmtxread"
MOGRIFY=$(which mogrify)

if [[ ! -x "$DMTXWRITE" ]]; then
   echo "Unable to execute \"$DMTXWRITE\""
   exit 1
fi

if [[ ! -x "$DMTXREAD" ]]; then
   echo "Unable to execute \"$DMTXREAD\""
   exit 1
fi

if [[ ! -x "$MOGRIFY" ]]; then
   echo "Unable to find or execute mogrify"
   exit 1
fi

ERROR_COUNT=0

echo "Generating and reading back barcodes from input messages"
echo "-----------------------------------------------------------------"

for file in input_messages/message_*.dat; do

   ENCODE=$(cat $file)
   MESSAGE=$(basename $file .dat | cut -d'_' -f2)

   for scheme in $SCHEMES; do

      OUTPUT="compare_generated/barcode_${MESSAGE}_${scheme}"
      $DMTXWRITE -e$scheme -o $OUTPUT.pnm $file 1>/dev/null 2>&1
      ERROR=$?

      if [[ "$ERROR" -eq 0 ]]; then
         mogrify -depth 8 -type TrueColor $OUTPUT.pnm

         DECODE=$($DMTXREAD $OUTPUT.pnm)

         if [[ "$ENCODE" != "$DECODE" ]]; then
            echo "message $MESSAGE scheme $scheme: FAIL"
            ERROR_COUNT=$[$ERROR_COUNT + 1]
         fi
      fi

   done
done

echo "$ERROR_COUNT error(s) encountered"
echo ""

exit 0
