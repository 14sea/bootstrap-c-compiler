char ob[8192]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

int classify(int n) {
  if (n < 0) return 0 - 1;
  else if (n == 0) return 0;
  else if (n < 10) return 1;
  else return 2;
}

int main() {
  int i; int s; int j;
  ps("classify "); pn(classify(-4)); pn(classify(0)); pn(classify(5)); pn(classify(99)); pc(10);
  s = 0; i = 0;
  while (i < 100) { i = i + 1; if (i % 3 == 0) continue; if (i > 50) break; s = s + i; }
  ps("while "); pn(s); pc(10);
  s = 0;
  for (i = 0; i < 10; i = i + 1) for (j = 0; j < 10; j = j + 1) s = s + i * j;
  ps("for "); pn(s); pc(10);
  s = 0;
  for (i = 0; ; i = i + 1) { if (i >= 20) break; if (i & 1) continue; s = s + i; }
  ps("forinf "); pn(s); pc(10);
  i = 0; s = 0;
  for (;;) { i = i + 1; s = s + i; if (i == 10) break; }
  ps("forever "); pn(s); pc(10);
  flush();
  return 0;
}
