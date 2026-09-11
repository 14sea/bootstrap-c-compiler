/* a slightly larger program: sorting, searching and an RPN calculator */
char ob[65536]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

long a[64];
long stack[64];
long sp;

long rnd;
long next() { rnd = (rnd * 1103515245 + 12345) & 0x7FFFFFFF; return rnd % 1000; }

void sort(long *v, long n) {
  long i; long j; long t;
  for (i = 0; i < n; i = i + 1)
    for (j = 0; j < n - 1 - i; j = j + 1)
      if (v[j] > v[j + 1]) { t = v[j]; v[j] = v[j + 1]; v[j + 1] = t; }
}

long bsearch(long *v, long n, long key) {
  long lo; long hi; long mid;
  lo = 0; hi = n - 1;
  while (lo <= hi) {
    mid = (lo + hi) / 2;
    if (v[mid] == key) return mid;
    if (v[mid] < key) lo = mid + 1; else hi = mid - 1;
  }
  return -1;
}

long rpn(char *s) {
  long x; long y;
  sp = 0;
  while (*s) {
    if (*s == ' ') { s = s + 1; continue; }
    if (*s >= '0' && *s <= '9') {
      x = 0;
      while (*s >= '0' && *s <= '9') { x = x * 10 + *s - '0'; s = s + 1; }
      stack[sp] = x; sp = sp + 1;
      continue;
    }
    sp = sp - 1; y = stack[sp];
    sp = sp - 1; x = stack[sp];
    if (*s == '+') x = x + y;
    else if (*s == '-') x = x - y;
    else if (*s == '*') x = x * y;
    else if (*s == '/') x = x / y;
    else if (*s == '%') x = x % y;
    stack[sp] = x; sp = sp + 1;
    s = s + 1;
  }
  return stack[0];
}

int main() {
  long i;
  rnd = 1;
  for (i = 0; i < 20; i = i + 1) a[i] = next();
  sort(a, 20);
  ps("sorted");
  for (i = 0; i < 20; i = i + 1) { pc(' '); pn(a[i]); }
  pc(10);
  ps("sortedok ");
  { long ok; ok = 1;
    for (i = 1; i < 20; i = i + 1) if (a[i - 1] > a[i]) ok = 0;
    pn(ok); }
  pc(10);
  ps("search "); pn(bsearch(a, 20, a[7])); pc(' '); pn(bsearch(a, 20, -5)); pc(10);
  ps("rpn "); pn(rpn("3 4 + 2 *")); pc(' '); pn(rpn("100 7 %")); pc(' ');
  pn(rpn("1 2 3 4 5 + + + +")); pc(' '); pn(rpn("10 2 / 3 -")); pc(10);
  flush();
  return 0;
}
