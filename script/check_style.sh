#!/bin/sh

LIBDMTX="$1"
if [[ -z "$LIBDMTX" ]]; then
   echo "Missing LIBDMTX paramter"
   exit 1
fi

if [[ ! -d "$LIBDMTX/script" ]]; then
   echo "Invalid LIBDMTX directory passed"
   exit 2
fi

set -x

$LIBDMTX/script/check_comments.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/check_copyright.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/check_keyword.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/check_license.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/check_spacing.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/check_whitespace.sh "$LIBDMTX" || exit 1
$LIBDMTX/script/run_perl.sh check_headers.pl "$LIBDMTX" || exit 1
$LIBDMTX/script/check_todo.sh "$LIBDMTX"

set +x

exit 0
