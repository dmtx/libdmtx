#!/bin/sh

function RunTest()
{
   SCRIPT="$1"
   SCRIPT_TYPE=$(echo "$SCRIPT" | awk -F'.' '{print $NF}')

   echo "   $SCRIPT"

   ERRORS=0
   for dir in $(find "$LIBDMTX" -type d); do

      if [[ "$dir" != "$LIBDMTX" &&
            "$dir" != "$LIBDMTX/test/simple_test" ]]; then
         continue
      fi

      for file in $(find $dir -maxdepth 1); do

         EXT=$(echo $file | awk -F'.' '{print $NF}')
         if [[ "$EXT" != "c" && "$EXT" != "h" && "$EXT" != "sh" && \
               "$EXT" != "py" && "$EXT" != "pl" ]]; then
            continue
         fi

         if [[ "$(basename $file)" = "config.h" ||
               "$(basename $file)" = "ltmain.sh" ]]; then
            continue
         fi

         if [[ $(cat $file | wc -l) -le 10 ]]; then
            #echo "      skipping \"$file\" (trivial file)"
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

RunTest check_comments.sh
RunTest check_copyright.sh
RunTest check_license.sh
RunTest check_spacing.sh
RunTest check_whitespace.sh
RunTest check_headers.pl
RunTest check_todo.sh

exit 0
