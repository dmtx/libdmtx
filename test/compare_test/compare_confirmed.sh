#!/bin/sh

ERROR_COUNT=0

echo "Comparing generated barcodes against confirmed results"
echo "-----------------------------------------------------------------"

for CONFIRMED in compare_confirmed/barcode_*_?.png; do

   GENERATED="compare_generated/$(basename $CONFIRMED | sed -e 's/^confirmed_/barcode_/')"
   if [[ ! -s "$GENERATED" ]]; then
      echo "FILE MISSING: Please run compare_generated.sh first."
      exit 1
   fi

   GENERATED_MD5SUM=$(convert -depth 8 -type TrueColor $GENERATED pnm: | md5sum)
   GENERATED_ERROR=$?

   CONFIRMED_MD5SUM=$(convert -depth 8 -type TrueColor $CONFIRMED pnm: | md5sum)
   CONFIRMED_ERROR=$?

   if [[ "$GENERATED_ERROR" -ne 0 || "$CONFIRMED_ERROR" -ne 0 ]]; then
      echo "Error: convert failed"
      exit 1
   fi

   if [[ "$GENERATED_MD5SUM" == "$CONFIRMED_MD5SUM" ]]; then
      echo "SUCCESS: $(basename $CONFIRMED)"
   else
      echo "FAILURE: $(basename $GENERATED)"
      ERROR_COUNT=$[$ERROR_COUNT + 1]
   fi

done

echo "$ERROR_COUNT difference(s) found"
echo ""

exit 0
