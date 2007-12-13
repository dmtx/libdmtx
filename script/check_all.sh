#!/bin/sh

function RunTest()
{
   SCRIPT="$1"
   SCRIPT_TYPE=$(echo "$SCRIPT" | awk -F'.' '{print $NF}')

   echo "   $SCRIPT"

   ERRORS=0
   for dir in $(find "$LIBDMTX" -type d); do

      if [[ "$dir" != "$LIBDMTX" &&
            "$dir" != "$LIBDMTX/util/dmtxread" &&
            "$dir" != "$LIBDMTX/util/dmtxwrite" &&
            "$dir" != "$LIBDMTX/test/gltest" &&
            "$dir" != "$LIBDMTX/test/simpletest" &&
            "$dir" != "$LIBDMTX/test/script" ]]; then
         continue
      fi

      for file in $(find $dir -maxdepth 1 -name "*.[ch]" -o -name "*.py"); do

         if [[ "$(basename $file)" = "config.h" ]]; then
            continue
         fi

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
