#!/bin/sh
# tests/run.sh [compiler]   -- compile every tests/*.c and diff against *.exp
cd "$(dirname "$0")/.."
CC=${1:-./build/cc}
mkdir -p build/t
fail=0
for c in tests/*.c; do
  b=$(basename "$c" .c)
  if ! "$CC" < "$c" > "build/t/$b.hasm" 2>"build/t/$b.err"; then
     echo "FAIL $b (compile)"; cat "build/t/$b.err"; fail=1; continue
  fi
  if ! ./build/hex0 < "build/t/$b.hasm" > "build/t/$b.hex" 2>"build/t/$b.err"; then
     echo "FAIL $b (assemble)"; cat "build/t/$b.err"; fail=1; continue
  fi
  rm -f "build/t/$b"
  if [ -x ./build/hex2bin ]; then
     if ! ./build/hex2bin < "build/t/$b.hex" > "build/t/$b" || ! chmod +x "build/t/$b"; then
        echo "FAIL $b (hex2bin)"; fail=1; continue
     fi
  else
     if ! env/bin/python tools/hex2bin.py "build/t/$b.hex" "build/t/$b"; then
        echo "FAIL $b (hex2bin)"; fail=1; continue
     fi
  fi
  "./build/t/$b" > "build/t/$b.out" 2>&1
  st=$?
  if [ "$st" != 0 ]; then
     echo "FAIL $b (exit status $st)"; fail=1; continue
  fi
  if cmp -s "build/t/$b.out" "tests/$b.exp"; then
     echo "ok   $b"
  else
     echo "FAIL $b (output)"; diff "tests/$b.exp" "build/t/$b.out" | head -20; fail=1
  fi
done
if [ $fail = 0 ]; then echo "all tests passed"; fi
exit $fail
