#!/bin/sh
# =====================================================================
#  build.sh -- rebuild everything from the hand written seed.
#
#  Nothing but the seed hexadecimal text and the source files in this
#  repository is used.  The only external program that touches a build
#  product is tools/hex2bin.py, a pure hexadecimal-text to byte
#  converter (and once the compiler exists, its own tools/hex2bin.c
#  does the same job, which the script verifies).
# =====================================================================
set -e
cd "$(dirname "$0")"
mkdir -p build

PY=env/bin/python
if [ ! -x "$PY" ]; then
    python3 -m venv env
fi

h2b() { "$PY" tools/hex2bin.py "$1" "$2"; }
say() { printf '\n\033[1m== %s\033[0m\n' "$1"; }

say "stage A  seed assembler  (hand written machine code)"
h2b seed/hexa.hex build/hexa
ls -l build/hexa

say "stage B  hex0  (assembler with named labels and macros)"
./build/hexa < stage1/hex0.hasm > build/hex0.hex
h2b build/hex0.hex build/hex0
ls -l build/hex0

say "stage C  cc0  (C subset compiler, written in hex0 assembly)"
cat stage1/prelude.hasm stage1/cc0.hasm | ./build/hex0 > build/cc0.hex
h2b build/cc0.hex build/cc0
ls -l build/cc0

say "stage D1 cc1 = cc0(cc.c)"
./build/cc0 < stage2/cc.c > build/cc1.hasm
./build/hex0 < build/cc1.hasm > build/cc1.hex
h2b build/cc1.hex build/cc1

say "stage D2 cc2 = cc1(cc.c)"
./build/cc1 < stage2/cc.c > build/cc2.hasm
./build/hex0 < build/cc2.hasm > build/cc2.hex
h2b build/cc2.hex build/cc2

say "stage D3 cc3 = cc2(cc.c)"
./build/cc2 < stage2/cc.c > build/cc3.hasm
./build/hex0 < build/cc3.hasm > build/cc3.hex
h2b build/cc3.hex build/cc3

say "fixpoint check"
# Each comparison is a standalone command so that set -e aborts the build on a
# mismatch.  Written as "cmp A B && echo ..." the failing cmp would be the left
# hand side of an && list, which set -e deliberately ignores, and a broken
# bootstrap would still report success.
cmp build/cc2 build/cc3
echo "cc2 == cc3   (the compiler reproduces itself exactly)"
cmp build/cc1 build/cc2
echo "cc1 == cc2   (cc0 and cc emit identical code as well)"
cp build/cc2 build/cc
md5sum build/cc1 build/cc2 build/cc3

say "self hosted hex2bin"
./build/cc < tools/hex2bin.c > build/hex2bin.hasm
./build/hex0 < build/hex2bin.hasm > build/hex2bin.hex
h2b build/hex2bin.hex build/hex2bin
for b in hexa hex0 cc0 cc1 cc2 cc3 hex2bin; do
    case $b in hexa) H=seed/hexa.hex;; *) H=build/$b.hex;; esac
    ./build/hex2bin < "$H" > build/.chk
    cmp -s build/.chk "build/$b" || { echo "hex2bin mismatch on $b"; exit 1; }
done
rm -f build/.chk
echo "our own hex2bin reproduces every binary byte for byte"

say "test suite"
./tests/run.sh ./build/cc

say "done"
echo "compiler:  build/cc      driver: tools/ccc.sh <src.c> <out>"
