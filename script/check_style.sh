#!/bin/sh

function RunTest()
{
   SCRIPT="$1"
   SCRIPT_TYPE=$(echo "$SCRIPT" | awk -F'.' '{print $NF}')

   echo "   $SCRIPT"

   ERRORS=0
   FILES=$(find "$LIBDMTX" -name "*.[ch]")
   for file in $FILES; do

      if [[ "$SCRIPT_TYPE" = "sh" ]]; then
         $LIBDMTX/script/$SCRIPT $file
         ERRORS=$(( ERRORS + $? ))
      elif [[ "$SCRIPT_TYPE" = "pl" ]]; then
         PERL=$(which perl)
         if [[ $? -ne 0 ]]; then
            echo "No perl interpreter found.  Skipping $SCRIPT test."
         else
            $PERL $LIBDMTX/script/$SCRIPT $file
            ERRORS=$(( ERRORS + $? ))
         fi
      fi

   done

   return $ERRORS
}

LIBDMTX="$1"
if [[ -z "$LIBDMTX" || ! -d "$LIBDMTX/script" ]]; then
   echo "Must provide valid LIBDMTX directory"
   exit 1
fi

RunTest check_comments.sh || exit 1
RunTest check_copyright.sh || exit 1
RunTest check_keyword.sh || exit 1
RunTest check_license.sh || exit 1
RunTest check_spacing.sh || exit 1
RunTest check_whitespace.sh || exit 1
RunTest check_headers.pl || exit 1
RunTest check_todo.sh

exit 0
