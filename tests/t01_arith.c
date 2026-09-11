char ob[8192]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void nl() { pc(10); }
void flush() { sys_write(1, ob, on); on = 0; }

int main() {
  ps("add "); pn(2 + 3); pc(' '); pn(-5 + 3); nl();
  ps("sub "); pn(10 - 4); pc(' '); pn(4 - 10); nl();
  ps("mul "); pn(6 * 7); pc(' '); pn(-6 * 7); nl();
  ps("div "); pn(100 / 7); pc(' '); pn(-100 / 7); nl();
  ps("mod "); pn(100 % 7); pc(' '); pn(-100 % 7); nl();
  ps("shift "); pn(1 << 20); pc(' '); pn(1048576 >> 10); pc(' '); pn(-16 >> 2); nl();
  ps("bits "); pn(0xF0 & 0x3C); pc(' '); pn(0xF0 | 0x0F); pc(' '); pn(0xFF ^ 0x0F); pc(' '); pn(~5); nl();
  ps("cmp "); pn(1 < 2); pn(2 < 1); pn(1 <= 1); pn(2 > 3); pn(3 >= 3); pn(4 == 4); pn(4 != 4); nl();
  ps("logic "); pn(0 && 1); pn(1 && 2); pn(0 || 0); pn(0 || 7); pn(!0); pn(!9); nl();
  ps("prec "); pn(2 + 3 * 4); pc(' '); pn((2 + 3) * 4); pc(' '); pn(1 + 2 < 4 == 1); nl();
  ps("big "); pn(1000000 * 1000000); nl();
  flush();
  return 0;
}
