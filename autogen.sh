#!/bin/sh

# Create empty m4 directory if missing
if [ ! -d "m4" ]; then
   echo "autogen.sh: creating empty m4 directory"
   mkdir m4
fi

echo "autogen.sh: running autoreconf"
autoreconf --force --install
