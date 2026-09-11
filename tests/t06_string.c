char ob[65536]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

char buf[256];
char *words[16];
int nwords;

long slen(char *s) { long n; n = 0; while (s[n]) n = n + 1; return n; }
long scmp(char *a, char *b) {
  while (*a && *a == *b) { a = a + 1; b = b + 1; }
  return *a - *b;
}
void rev(char *s) {
  long i; long j; char t;
  i = 0; j = slen(s) - 1;
  while (i < j) { t = s[i]; s[i] = s[j]; s[j] = t; i = i + 1; j = j - 1; }
}
void split(char *s) {
  nwords = 0;
  while (*s) {
    while (*s == ' ') s = s + 1;
    if (!*s) break;
    words[nwords] = s;
    nwords = nwords + 1;
    while (*s && *s != ' ') s = s + 1;
    if (*s) { *s = 0; s = s + 1; }
  }
}
void scopy(char *d, char *s) { while (*s) { *d = *s; d = d + 1; s = s + 1; } *d = 0; }

int main() {
  int i;
  scopy(buf, "the quick brown fox");
  ps("len "); pn(slen(buf)); pc(10);
  ps("cmp "); pn(scmp("abc", "abc")); pc(' '); pn(scmp("abc", "abd") < 0); pc(' '); pn(scmp("b", "a") > 0); pc(10);
  split(buf);
  ps("split "); pn(nwords); pc(':');
  for (i = 0; i < nwords; i = i + 1) { pc(' '); ps(words[i]); }
  pc(10);
  scopy(buf, "abcdefg");
  rev(buf);
  ps("rev "); ps(buf); pc(10);
  flush();
  return 0;
}
