#!/bin/sh

ERROR_COUNT=0

for PNG_BARCODE in compare_siemens/siemens_*_?.png; do
   PNM_BARCODE=$(echo $PNG_BARCODE | sed -e 's/\.png$/.pnm/')
   convert -depth 8 -type TrueColor $PNG_BARCODE $PNM_BARCODE
done

echo "Comparing generated barcodes against Seimens results"
echo "-----------------------------------------------------------------"

for file in compare_siemens/siemens_*_?.pnm; do

   TEST_BARCODE=compare_generated/$(basename $file | sed -e 's/^siemens_/barcode_/')
   if [[ ! -r "$TEST_BARCODE" ]]; then
      continue
   fi

   cmp $file $TEST_BARCODE
   if [[ $? -ne 0 ]]; then
      ERROR_COUNT=$[$ERROR_COUNT + 1]
   fi

done

echo "$ERROR_COUNT difference(s) found"
echo ""

exit 0
