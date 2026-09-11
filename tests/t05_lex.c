char ob[8192]; int on;
void pc(int c) { ob[on] = c; on = on + 1; }
void ps(char *s) { while (*s) { pc(*s); s = s + 1; } }
void pn(long n) { if (n < 0) { pc('-'); n = 0 - n; } if (n > 9) pn(n / 10); pc('0' + n % 10); }
void flush() { sys_write(1, ob, on); on = 0; }

/* a block comment
   spanning several lines */
int main() {
  // a line comment
  ps("esc ");
  pn('\n'); pc(' '); pn('\t'); pc(' '); pn('\\'); pc(' '); pn('\''); pc(' '); pn('"'); pc(' '); pn('\0'); pc(10);
  ps("num "); pn(0); pc(' '); pn(255); pc(' '); pn(0xff); pc(' '); pn(0xDEADBEEF); pc(' '); pn(0X10); pc(10);
  ps("str ");
  ps("tab[\t]nl[");  /* embedded escapes */
  pc(']'); pc(10);
  ps("chr "); pc('a'); pc('B'); pc('9'); pc('_'); pc(10);
  flush();
  return 0;
}
