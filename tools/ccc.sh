#!/bin/sh
# ccc.sh <source.c> <output>   -- compile, assemble and link in one step
set -e
ROOT=$(cd "$(dirname "$0")/.." && pwd)
CC=${CC:-$ROOT/build/cc}
"$CC" < "$1" > "$2.hasm"
"$ROOT/build/hex0" < "$2.hasm" > "$2.hex"
if [ -x "$ROOT/build/hex2bin" ]; then
    "$ROOT/build/hex2bin" < "$2.hex" > "$2"
else
    "$ROOT/env/bin/python" "$ROOT/tools/hex2bin.py" "$2.hex" "$2"
fi
chmod +x "$2"
