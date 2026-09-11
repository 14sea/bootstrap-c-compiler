/* hex2bin.c -- the hexadecimal-text to binary converter, written in the
 * compiler's own C subset and built by the compiler itself.  It does the
 * same job as tools/hex2bin.py, which is only needed to create the very
 * first seed binary.
 *
 *     ./hex2bin < prog.hex > prog   (then chmod +x prog)
 */
char inb[48000000];
char oub[16000000];

long hexv(long c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 55;
    if (c >= 'a' && c <= 'f') return c - 87;
    return -1;
}

void wstr(char *s)
{
    long n;
    n = 0;
    while (s[n]) n = n + 1;
    sys_write(2, s, n);
}

int main(int argc, char **argv)
{
    long n; long t; long i; long o; long c; long hi; long lo;
    t = 0;
    while (1) {
        n = sys_read(0, inb + t, 1000000);
        if (n <= 0) break;
        t = t + n;
    }
    i = 0;
    o = 0;
    while (i < t) {
        c = inb[i];
        if (c == '#' || c == ';') {
            while (i < t && inb[i] != 10) i = i + 1;
            continue;
        }
        if (c <= ' ') { i = i + 1; continue; }
        hi = hexv(c);
        lo = -1;
        if (i + 1 < t) lo = hexv(inb[i + 1]);
        if (hi < 0 || lo < 0) { wstr("hex2bin: not a hexadecimal byte\n"); sys_exit(1); }
        oub[o] = hi * 16 + lo;
        o = o + 1;
        i = i + 2;
    }
    sys_write(1, oub, o);
    return 0;
}
