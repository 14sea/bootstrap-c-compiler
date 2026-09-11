char ob[8192]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

long six(long a, long b, long c, long d, long e, long f) {
  return a * 100000 + b * 10000 + c * 1000 + d * 100 + e * 10 + f;
}
long fact(long n) { if (n <= 1) return 1; return n * fact(n - 1); }
long ack(long m, long n) {
  if (m == 0) return n + 1;
  if (n == 0) return ack(m - 1, 1);
  return ack(m - 1, ack(m, n - 1));
}
long even(long n);
long odd(long n) { if (n == 0) return 0; return even(n - 1); }
long even(long n) { if (n == 0) return 1; return odd(n - 1); }

int main() {
  ps("six "); pn(six(1, 2, 3, 4, 5, 6)); pc(10);
  ps("fact "); pn(fact(20)); pc(10);
  ps("ack "); pn(ack(2, 3)); pc(' '); pn(ack(3, 3)); pc(10);
  ps("mutual "); pn(even(10)); pn(odd(10)); pn(even(7)); pn(odd(7)); pc(10);
  ps("later "); pn(defined_later(6)); pc(10);
  flush();
  return 0;
}

long defined_later(long x) { return x * x; }
