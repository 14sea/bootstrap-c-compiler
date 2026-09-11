#!/usr/bin/env python3
# The ONLY python in this project.
# Pure text->binary conversion: reads a file of hexadecimal byte tokens
# (whitespace separated, '#' or ';' start a comment running to end of line)
# and writes the corresponding bytes.  It performs no assembly, no symbol
# resolution, no code generation -- every token must already be a literal
# two-digit byte.
import sys, os

def main():
    if len(sys.argv) != 3:
        sys.stderr.write("usage: hex2bin.py <in.hex> <out.bin>\n")
        return 1
    data = bytearray()
    with open(sys.argv[1], "r") as f:
        for lineno, line in enumerate(f, 1):
            for cut in ("#", ";"):
                i = line.find(cut)
                if i >= 0:
                    line = line[:i]
            for tok in line.split():
                if len(tok) != 2:
                    sys.stderr.write("%s:%d: not a byte: %r\n" % (sys.argv[1], lineno, tok))
                    return 1
                data.append(int(tok, 16))
    with open(sys.argv[2], "wb") as f:
        f.write(bytes(data))
    os.chmod(sys.argv[2], 0o755)
    return 0

sys.exit(main())
