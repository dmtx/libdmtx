#!/bin/sh

#SCHEMES="b f a c t x e 8"
SCHEMES="b a c t x e 8"
DMTXWRITE="$(which dmtxwrite)"
DMTXREAD="$(which dmtxread)"
MOGRIFY=$(which mogrify)
COMPARE_DIR="compare_generated"

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

if [[ ! -d "$COMPARE_DIR" ]]; then
   $(which mkdir) "$COMPARE_DIR"
fi

ERROR_COUNT=0

echo "Generating and reading back barcodes from input messages"
echo "-----------------------------------------------------------------"

for file in input_messages/message_*.dat; do

   ENCODE=$(cat $file)
   MESSAGE=$(basename $file .dat | cut -d'_' -f2)

   for scheme in $SCHEMES; do

      OUTPUT="${COMPARE_DIR}/barcode_${MESSAGE}_${scheme}"
      $DMTXWRITE -e$scheme -o ${OUTPUT}.png $file 1>/dev/null 2>&1
      ERROR=$?
      if [[ "$ERROR" -eq 70 ]]; then
         # XXX revisit this to use more specific error code when available
         echo "   SKIP: message $MESSAGE scheme ${scheme} (unsupported character)"
         continue;
      elif [[ "$ERROR" -ne 0 && "$ERROR" -ne 70 ]]; then
         echo "  ERROR: dmtxwrite failed"
         exit "$ERROR";
      fi

      $MOGRIFY -depth 8 -type TrueColor ${OUTPUT}.png
      ERROR=$?
      if [[ $? -ne 0 ]]; then
         echo "  ERROR: mogrify failed"
         exit "$ERROR";
      fi

      DECODE=$($DMTXREAD ${OUTPUT}.png)
      ERROR=$?
      if [[ $? -ne 0 ]]; then
         echo " ERROR: dmtxread failed"
         exit "$ERROR";
      fi

      if [[ "$ENCODE" == "$DECODE" ]]; then
         echo "SUCCESS: message $MESSAGE scheme ${scheme}"
      else
         echo "FAILURE: message $MESSAGE scheme ${scheme}"
         ERROR_COUNT=$[$ERROR_COUNT + 1]
      fi

   done
done

echo "$ERROR_COUNT error(s) encountered"
echo ""

exit 0
