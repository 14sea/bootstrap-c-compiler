char ob[8192]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

int grid[100];
char text[64];

long slen(char *s) { char *p; p = s; while (*p) p = p + 1; return p - s; }
void scopy(char *d, char *s) { while (*s) { *d = *s; d = d + 1; s = s + 1; } *d = 0; }
void swap(long *a, long *b) { long t; t = *a; *a = *b; *b = t; }

int main() {
  int i; long x; long y; long *p; char *c;
  for (i = 0; i < 100; i = i + 1) grid[i] = i * 3;
  ps("array "); pn(grid[0]); pc(' '); pn(grid[37]); pc(' '); pn(grid[99]); pc(10);
  p = grid;
  ps("deref "); pn(*p); pc(' '); pn(*(p + 10)); pc(' '); pn(p[20]); pc(10);
  ps("pdiff "); pn(&grid[40] - &grid[10]); pc(10);
  x = 11; y = 22;
  swap(&x, &y);
  ps("swap "); pn(x); pc(' '); pn(y); pc(10);
  scopy(text, "a string of chars");
  ps("copy "); ps(text); pc(' '); pn(slen(text)); pc(10);
  c = text;
  ps("chars "); pn(c[0]); pc(' '); pn(*(c + 2)); pc(' '); pn(c[slen(text) - 1]); pc(10);
  ps("2d ");
  { char **av; char *m[4]; m[0] = "zero"; m[1] = "one"; m[2] = "two"; m[3] = 0;
    av = m; i = 0;
    while (av[i]) { ps(av[i]); pc('/'); i = i + 1; } }
  pc(10);
  flush();
  return 0;
}
