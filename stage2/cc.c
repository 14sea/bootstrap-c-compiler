/* =====================================================================
 *  cc.c -- a self hosting compiler for a subset of C.
 *
 *  Reads a C source file on stdin, writes hex0 assembly text on stdout.
 *  That text is turned into an ELF executable by  hex0  (label resolver)
 *  and  hex2bin  (hexadecimal text to bytes).
 *
 *  The subset the compiler accepts, and the subset this file is written
 *  in, are described in README.md.  No preprocessor, no structs, no
 *  floating point.  char is 1 byte, int / long / pointers are 8 bytes.
 *
 *  Types are encoded as one integer:
 *        0      char
 *        1      int / long / void
 *        t + 2  pointer to t
 *  so the size of t is 1 when t is 0, and 8 otherwise.
 * ===================================================================== */

/* ------------------------------ storage --------------------------- */
char src[4000000];          /* the whole source file                   */
char out[16000000];         /* assembly text produced so far           */
char sbuf[1000000];         /* bytes of all string literals            */
char forbuf[262144];        /* holds "step" code while a for body is   */
                            /* being compiled                          */

char *srcp;                 /* lexer cursor                            */
long outn;                  /* bytes used in out                       */
long forsp;                 /* bytes used in forbuf                    */

long tk;                    /* token kind: 0 eof 1 ident 2 num 3 str 4 punct */
long tv;                    /* token value                             */
char *tp;                   /* identifier text                         */
long tl;                    /* identifier length                       */
long line;                  /* current source line                     */

char *gname[4096];
long glen[4096];
long gtype[4096];
long gkind[4096];           /* 0 variable, 1 array, 2 function         */
long gval[4096];            /* absolute address                        */
long nglb;

char *lname[1024];
long llen[1024];
long ltype[1024];
long lkind[1024];
long lval[1024];            /* frame offset, always negative           */
long nloc;

long sstart[8192];          /* offset of each string literal in sbuf   */
long slen[8192];
long nstr;
long sofs;

long lblno;                 /* label counter                           */
long goff;                  /* next free byte of global storage        */
long loff;                  /* bytes of frame used by the function     */
long curty;                 /* type of the value just compiled         */
long curlv;                 /* 1 when rax holds the address of an lvalue */
long brklbl;
long cntlbl;

/* ------------------------------ output ---------------------------- */
void ech(long c)
{
    out[outn] = c;
    outn = outn + 1;
}

void es(char *s)
{
    while (*s) {
        ech(*s);
        s = s + 1;
    }
}

long hexc(long n)
{
    if (n < 10) return n + '0';
    return n + 55;
}

void ehb(long b)
{
    ech(hexc((b >> 4) & 15));
    ech(hexc(b & 15));
    ech(' ');
}

void enb(long v, long n)
{
    while (n > 0) {
        ehb(v & 255);
        v = v >> 8;
        n = n - 1;
    }
}

void edec(long n)
{
    if (n >= 10) edec(n / 10);
    ech(n % 10 + '0');
}

/* ------------------------------ errors ---------------------------- */
void wstr(char *s)
{
    long n;
    n = 0;
    while (s[n]) n = n + 1;
    sys_write(2, s, n);
}

void wdec(long n)
{
    char b[8];
    if (n >= 10) wdec(n / 10);
    b[0] = n % 10 + '0';
    sys_write(2, b, 1);
}

void die(char *m)
{
    wstr("cc: ");
    wstr(m);
    wstr(" near line ");
    wdec(line);
    wstr("\n");
    sys_exit(1);
}

/* ------------------------------ lexer ----------------------------- */
long isalp(long c)
{
    if (c >= 'a' && c <= 'z') return 1;
    if (c >= 'A' && c <= 'Z') return 1;
    if (c == '_') return 1;
    return 0;
}

long isdig(long c)
{
    if (c >= '0' && c <= '9') return 1;
    return 0;
}

long isaln(long c)
{
    if (isalp(c)) return 1;
    return isdig(c);
}

long hexv(long c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 55;
    if (c >= 'a' && c <= 'f') return c - 87;
    return -1;
}

long resc()
{
    long c;
    c = *srcp;
    srcp = srcp + 1;
    if (c != 92) return c;
    c = *srcp;
    srcp = srcp + 1;
    if (c == 'n') return 10;
    if (c == 't') return 9;
    if (c == 'r') return 13;
    if (c == '0') return 0;
    return c;
}

void lex()
{
    long c;
    long c2;
    long n;
    while (1) {
        c = *srcp;
        if (c == 0) break;
        if (c == 10) { line = line + 1; srcp = srcp + 1; continue; }
        if (c <= ' ') { srcp = srcp + 1; continue; }
        if (c == '/' && srcp[1] == '/') {
            while (*srcp != 0 && *srcp != 10) srcp = srcp + 1;
            continue;
        }
        if (c == '/' && srcp[1] == '*') {
            srcp = srcp + 2;
            while (*srcp != 0) {
                if (*srcp == 10) line = line + 1;
                if (*srcp == '*' && srcp[1] == '/') { srcp = srcp + 2; break; }
                srcp = srcp + 1;
            }
            continue;
        }
        break;
    }
    c = *srcp;
    if (c == 0) { tk = 0; return; }
    if (isalp(c)) {
        tp = srcp;
        while (isaln(*srcp)) srcp = srcp + 1;
        tl = srcp - tp;
        tk = 1;
        return;
    }
    if (isdig(c)) {
        tv = 0;
        if (c == '0' && (srcp[1] == 'x' || srcp[1] == 'X')) {
            srcp = srcp + 2;
            while (hexv(*srcp) >= 0) {
                tv = tv * 16 + hexv(*srcp);
                srcp = srcp + 1;
            }
        } else {
            while (isdig(*srcp)) {
                tv = tv * 10 + *srcp - '0';
                srcp = srcp + 1;
            }
        }
        tk = 2;
        return;
    }
    if (c == 39) {
        srcp = srcp + 1;
        tv = resc();
        if (*srcp != 39) die("bad character literal");
        srcp = srcp + 1;
        tk = 2;
        return;
    }
    if (c == '"') {
        srcp = srcp + 1;
        tv = nstr;
        sstart[nstr] = sofs;
        n = 0;
        while (*srcp != 0 && *srcp != '"') {
            sbuf[sofs + n] = resc();
            n = n + 1;
        }
        if (*srcp == 0) die("unterminated string literal");
        srcp = srcp + 1;
        sbuf[sofs + n] = 0;
        n = n + 1;
        slen[nstr] = n;
        sofs = sofs + n;
        nstr = nstr + 1;
        tk = 3;
        return;
    }
    tk = 4;
    c2 = srcp[1];
    if (c == '=' && c2 == '=') { tv = 1; srcp = srcp + 2; return; }
    if (c == '!' && c2 == '=') { tv = 2; srcp = srcp + 2; return; }
    if (c == '<' && c2 == '=') { tv = 3; srcp = srcp + 2; return; }
    if (c == '>' && c2 == '=') { tv = 4; srcp = srcp + 2; return; }
    if (c == '&' && c2 == '&') { tv = 5; srcp = srcp + 2; return; }
    if (c == '|' && c2 == '|') { tv = 6; srcp = srcp + 2; return; }
    if (c == '<' && c2 == '<') { tv = 7; srcp = srcp + 2; return; }
    if (c == '>' && c2 == '>') { tv = 8; srcp = srcp + 2; return; }
    tv = c;
    srcp = srcp + 1;
}

long isp(long code)
{
    if (tk != 4) return 0;
    if (tv != code) return 0;
    return 1;
}

void expect(long code)
{
    if (!isp(code)) die("unexpected token");
    lex();
}

long tokeq(char *s)
{
    long n;
    if (tk != 1) return 0;
    n = 0;
    while (s[n]) n = n + 1;
    if (n != tl) return 0;
    n = 0;
    while (n < tl) {
        if (tp[n] != s[n]) return 0;
        n = n + 1;
    }
    return 1;
}

long meq(char *a, char *b, long n)
{
    long i;
    i = 0;
    while (i < n) {
        if (a[i] != b[i]) return 0;
        i = i + 1;
    }
    return 1;
}

/* --------------------------- symbol table ------------------------- */
/* handles: 1..nloc for locals, -(1..nglb) for globals, 0 for missing  */
long sfind(char *p, long l)
{
    long i;
    i = nloc;
    while (i > 0) {
        i = i - 1;
        if (llen[i] == l && meq(p, lname[i], l)) return i + 1;
    }
    i = nglb;
    while (i > 0) {
        i = i - 1;
        if (glen[i] == l && meq(p, gname[i], l)) return 0 - (i + 1);
    }
    return 0;
}

long symtype(long h)
{
    if (h > 0) return ltype[h - 1];
    return gtype[0 - h - 1];
}

long symkind(long h)
{
    if (h > 0) return lkind[h - 1];
    return gkind[0 - h - 1];
}

long symval(long h)
{
    if (h > 0) return lval[h - 1];
    return gval[0 - h - 1];
}

void newglb(char *p, long l, long type, long kind, long val)
{
    if (nglb >= 4096) die("too many globals");
    gname[nglb] = p;
    glen[nglb] = l;
    gtype[nglb] = type;
    gkind[nglb] = kind;
    gval[nglb] = val;
    nglb = nglb + 1;
}

void newloc(char *p, long l, long type, long kind, long val)
{
    if (nloc >= 1024) die("too many locals");
    lname[nloc] = p;
    llen[nloc] = l;
    ltype[nloc] = type;
    lkind[nloc] = kind;
    lval[nloc] = val;
    nloc = nloc + 1;
}

/* ------------------------------ labels ---------------------------- */
long newlbl()
{
    lblno = lblno + 1;
    return lblno;
}

void deflbl(long n)
{
    ech(':');
    ech('L');
    edec(n);
    ech(10);
}

void refl(long n)
{
    ech('@');
    ech('L');
    edec(n);
    ech(' ');
}

void gjmp(long n)
{
    es("E9 ");
    refl(n);
}

void gje(long n)
{
    es("0F 84 ");
    refl(n);
}

void gjne(long n)
{
    es("0F 85 ");
    refl(n);
}

/* --------------------------- expressions -------------------------- */
void expr(long minprec);

void load()
{
    if (!curlv) return;
    if (curty == 0) es("48 0F BE 00 ");     /* movsx rax,byte [rax]    */
    else es("48 8B 00 ");                   /* mov   rax,[rax]         */
    curlv = 0;
}

long tysize(long t)
{
    if (t == 0) return 1;
    return 8;
}

long builtin(char *p, long l)
{
    if (l == 8 && meq(p, "sys_read", 8)) return 0;
    if (l == 9 && meq(p, "sys_write", 9)) return 1;
    if (l == 8 && meq(p, "sys_open", 8)) return 2;
    if (l == 9 && meq(p, "sys_close", 9)) return 3;
    if (l == 9 && meq(p, "sys_lseek", 9)) return 8;
    if (l == 8 && meq(p, "sys_exit", 8)) return 60;
    return -1;
}

void emitname(char *p, long l)
{
    long i;
    i = 0;
    while (i < l) {
        ech(p[i]);
        i = i + 1;
    }
}

void gcall(char *p, long l)
{
    long n;
    long b;
    lex();
    n = 0;
    while (!isp(')')) {
        expr(1);
        load();
        es("50 ");
        n = n + 1;
        if (!isp(',')) break;
        lex();
    }
    expect(')');
    if (n > 6) die("too many arguments");
    while (n > 0) {
        n = n - 1;
        if (n == 0) es("5F ");
        if (n == 1) es("5E ");
        if (n == 2) es("5A ");
        if (n == 3) es("59 ");
        if (n == 4) es("41 58 ");
        if (n == 5) es("41 59 ");
    }
    b = builtin(p, l);
    if (b >= 0) {
        es("B8 ");
        enb(b, 4);
        es("0F 05 ");
    } else {
        es("E8 @F_");
        emitname(p, l);
        ech(' ');
    }
    curty = 1;
    curlv = 0;
}

void primary()
{
    char *p;
    long l;
    long h;
    long v;
    if (tk == 2) {
        es("48 B8 ");
        enb(tv, 8);
        lex();
        curty = 1;
        curlv = 0;
        return;
    }
    if (tk == 3) {
        es("B8 &S");
        edec(tv);
        ech(' ');
        lex();
        curty = 2;
        curlv = 0;
        return;
    }
    if (isp('(')) {
        lex();
        expr(1);
        expect(')');
        return;
    }
    if (tk != 1) die("bad expression");
    p = tp;
    l = tl;
    lex();
    if (isp('(')) {
        gcall(p, l);
        return;
    }
    h = sfind(p, l);
    if (h == 0) die("unknown identifier");
    if (symkind(h) == 2) die("function name used as a value");
    v = symval(h);
    if (v < 0) {
        es("48 8D 85 ");                    /* lea rax,[rbp+disp32]    */
        enb(v, 4);
    } else {
        es("B8 ");                          /* mov eax,address         */
        enb(v, 4);
    }
    if (symkind(h) == 1) {
        curty = symtype(h) + 2;
        curlv = 0;
    } else {
        curty = symtype(h);
        curlv = 1;
    }
}

void postfix()
{
    long bt;
    primary();
    while (isp('[')) {
        lex();
        load();
        if (curty < 2) die("pointer required before [");
        bt = curty;
        es("50 ");
        expr(1);
        load();
        if (tysize(bt - 2) == 8) es("48 C1 E0 03 ");
        es("48 89 C1 58 48 01 C8 ");        /* rcx=idx; rax=base; add  */
        expect(']');
        curty = bt - 2;
        curlv = 1;
    }
}

void unary()
{
    if (tk == 4) {
        if (tv == '-') {
            lex(); unary(); load();
            es("48 F7 D8 ");
            curty = 1; curlv = 0;
            return;
        }
        if (tv == '!') {
            lex(); unary(); load();
            es("48 83 F8 00 0F 94 C0 0F B6 C0 ");
            curty = 1; curlv = 0;
            return;
        }
        if (tv == '~') {
            lex(); unary(); load();
            es("48 F7 D0 ");
            curty = 1; curlv = 0;
            return;
        }
        if (tv == '*') {
            lex(); unary(); load();
            if (curty < 2) die("pointer required after *");
            curty = curty - 2;
            curlv = 1;
            return;
        }
        if (tv == '&') {
            lex(); unary();
            if (!curlv) die("lvalue required after &");
            curlv = 0;
            curty = curty + 2;
            return;
        }
        if (tv == '+') {
            lex(); unary();
            return;
        }
    }
    postfix();
}

long prec(long op)
{
    if (op == 6) return 2;
    if (op == 5) return 3;
    if (op == '|') return 4;
    if (op == '^') return 5;
    if (op == '&') return 6;
    if (op == 1 || op == 2) return 7;
    if (op == '<' || op == '>' || op == 3 || op == 4) return 8;
    if (op == 7 || op == 8) return 9;
    if (op == '+' || op == '-') return 10;
    if (op == '*' || op == '/' || op == '%') return 11;
    return 0;
}

/* rax = left operand, rcx = right operand */
void genbin(long op, long lt, long rt)
{
    if (op == '+') {
        if (lt >= 2 && rt < 2) {
            if (tysize(lt - 2) == 8) es("48 C1 E1 03 ");
            es("48 01 C8 ");
            curty = lt;
        } else if (lt < 2 && rt >= 2) {
            if (tysize(rt - 2) == 8) es("48 C1 E0 03 ");
            es("48 01 C8 ");
            curty = rt;
        } else {
            es("48 01 C8 ");
            curty = 1;
        }
        curlv = 0;
        return;
    }
    if (op == '-') {
        if (lt >= 2 && rt < 2) {
            if (tysize(lt - 2) == 8) es("48 C1 E1 03 ");
            es("48 29 C8 ");
            curty = lt;
        } else if (lt >= 2 && rt >= 2) {
            es("48 29 C8 ");
            if (tysize(lt - 2) == 8) es("48 C1 F8 03 ");
            curty = 1;
        } else {
            es("48 29 C8 ");
            curty = 1;
        }
        curlv = 0;
        return;
    }
    curty = 1;
    curlv = 0;
    if (op == '*') { es("48 0F AF C1 "); return; }
    if (op == '/') { es("48 99 48 F7 F9 "); return; }
    if (op == '%') { es("48 99 48 F7 F9 48 89 D0 "); return; }
    if (op == '&') { es("48 21 C8 "); return; }
    if (op == '|') { es("48 09 C8 "); return; }
    if (op == '^') { es("48 31 C8 "); return; }
    if (op == 7) { es("48 D3 E0 "); return; }
    if (op == 8) { es("48 D3 F8 "); return; }
    es("48 39 C8 ");
    if (op == 1) { es("0F 94 C0 0F B6 C0 "); return; }
    if (op == 2) { es("0F 95 C0 0F B6 C0 "); return; }
    if (op == '<') { es("0F 9C C0 0F B6 C0 "); return; }
    if (op == '>') { es("0F 9F C0 0F B6 C0 "); return; }
    if (op == 3) { es("0F 9E C0 0F B6 C0 "); return; }
    if (op == 4) { es("0F 9D C0 0F B6 C0 "); return; }
    die("bad operator");
}

void expr(long minprec)
{
    long op;
    long p;
    long lt;
    long rt;
    long l1;
    long l2;
    unary();
    while (tk == 4) {
        op = tv;
        if (op == '=') {
            if (minprec > 1) return;
            if (!curlv) die("lvalue required before =");
            lt = curty;
            es("50 ");
            lex();
            expr(1);
            load();
            es("59 ");
            if (lt == 0) es("88 01 ");
            else es("48 89 01 ");
            curty = lt;
            curlv = 0;
            continue;
        }
        p = prec(op);
        if (p == 0 || p < minprec) return;
        lex();
        if (op == 5) {
            load();
            l1 = newlbl();
            es("48 85 C0 "); gje(l1);
            expr(p + 1); load();
            es("48 85 C0 "); gje(l1);
            es("B8 01 00 00 00 ");
            l2 = newlbl();
            gjmp(l2);
            deflbl(l1);
            es("31 C0 ");
            deflbl(l2);
            curty = 1; curlv = 0;
            continue;
        }
        if (op == 6) {
            load();
            l1 = newlbl();
            es("48 85 C0 "); gjne(l1);
            expr(p + 1); load();
            es("48 85 C0 "); gjne(l1);
            es("31 C0 ");
            l2 = newlbl();
            gjmp(l2);
            deflbl(l1);
            es("B8 01 00 00 00 ");
            deflbl(l2);
            curty = 1; curlv = 0;
            continue;
        }
        load();
        lt = curty;
        es("50 ");
        expr(p + 1);
        load();
        rt = curty;
        es("48 89 C1 58 ");
        genbin(op, lt, rt);
    }
}

/* --------------------------- declarations ------------------------- */
long ptype()
{
    long t;
    if (tk != 1) return -1;
    if (tokeq("char")) t = 0;
    else if (tokeq("int")) t = 1;
    else if (tokeq("long")) t = 1;
    else if (tokeq("void")) t = 1;
    else return -1;
    lex();
    while (isp('*')) {
        t = t + 2;
        lex();
    }
    return t;
}

void stmt();

void blkitem()
{
    long t;
    long n;
    long kind;
    long size;
    char *p;
    long l;
    t = ptype();
    if (t < 0) {
        stmt();
        return;
    }
    while (1) {
        if (tk != 1) die("identifier expected");
        p = tp;
        l = tl;
        lex();
        n = 1;
        kind = 0;
        if (isp('[')) {
            lex();
            n = tv;
            lex();
            expect(']');
            kind = 1;
        }
        if (t == 0) size = (n + 7) & 0 - 8;
        else size = n * 8;
        loff = loff + size;
        if (loff > 4096) die("function frame overflow");
        newloc(p, l, t, kind, 0 - loff);
        if (!isp(',')) break;
        lex();
    }
    expect(';');
}

void stmt()
{
    long l1;
    long l2;
    long l3;
    long ob;
    long oc;
    long mark;
    long n;
    long i;
    if (isp('{')) {
        lex();
        while (!isp('}')) {
            if (tk == 0) die("unexpected end of file");
            blkitem();
        }
        lex();
        return;
    }
    if (isp(';')) { lex(); return; }
    if (tk == 1) {
        if (tokeq("if")) {
            lex(); expect('('); expr(1); load(); expect(')');
            l1 = newlbl();
            es("48 85 C0 ");
            gje(l1);
            stmt();
            if (tokeq("else")) {
                l2 = newlbl();
                gjmp(l2);
                deflbl(l1);
                lex();
                stmt();
                deflbl(l2);
            } else {
                deflbl(l1);
            }
            return;
        }
        if (tokeq("while")) {
            lex();
            l1 = newlbl();
            l2 = newlbl();
            ob = brklbl; oc = cntlbl;
            brklbl = l2; cntlbl = l1;
            deflbl(l1);
            expect('('); expr(1); load(); expect(')');
            es("48 85 C0 ");
            gje(l2);
            stmt();
            gjmp(l1);
            deflbl(l2);
            brklbl = ob; cntlbl = oc;
            return;
        }
        if (tokeq("for")) {
            lex();
            expect('(');
            if (!isp(';')) { expr(1); load(); }
            expect(';');
            l1 = newlbl();
            l2 = newlbl();
            l3 = newlbl();
            ob = brklbl; oc = cntlbl;
            brklbl = l3; cntlbl = l2;
            deflbl(l1);
            if (!isp(';')) {
                expr(1);
                load();
                es("48 85 C0 ");
                gje(l3);
            }
            expect(';');
            mark = outn;
            if (!isp(')')) { expr(1); load(); }
            n = outn - mark;
            i = 0;
            while (i < n) {
                forbuf[forsp + i] = out[mark + i];
                i = i + 1;
            }
            outn = mark;
            forsp = forsp + n;
            expect(')');
            stmt();
            deflbl(l2);
            forsp = forsp - n;
            i = 0;
            while (i < n) {
                ech(forbuf[forsp + i]);
                i = i + 1;
            }
            gjmp(l1);
            deflbl(l3);
            brklbl = ob; cntlbl = oc;
            return;
        }
        if (tokeq("return")) {
            lex();
            if (!isp(';')) { expr(1); load(); }
            expect(';');
            es("48 89 EC 5D C3 ");
            return;
        }
        if (tokeq("break")) {
            lex();
            expect(';');
            gjmp(brklbl);
            return;
        }
        if (tokeq("continue")) {
            lex();
            expect(';');
            gjmp(cntlbl);
            return;
        }
    }
    expr(1);
    load();
    expect(';');
}

void decl()
{
    long t;
    long pt;
    long n;
    long kind;
    long size;
    long np;
    long i;
    char *p;
    char *pp;
    long l;
    long pl;
    t = ptype();
    if (t < 0) die("type expected");
    if (tk != 1) die("identifier expected");
    p = tp;
    l = tl;
    lex();
    if (!isp('(')) {
        while (1) {
            n = 1;
            kind = 0;
            if (isp('[')) {
                lex();
                n = tv;
                lex();
                expect(']');
                kind = 1;
            }
            if (t == 0) size = (n + 7) & 0 - 8;
            else size = n * 8;
            newglb(p, l, t, kind, 134217728 + goff);
            goff = goff + size;
            if (!isp(',')) break;
            lex();
            if (tk != 1) die("identifier expected");
            p = tp;
            l = tl;
            lex();
        }
        expect(';');
        return;
    }
    newglb(p, l, t, 2, 0);
    lex();
    nloc = 0;
    loff = 0;
    np = 0;
    while (!isp(')')) {
        pt = ptype();
        if (pt < 0) die("type expected");
        if (tk != 1) die("identifier expected");
        pp = tp;
        pl = tl;
        lex();
        loff = loff + 8;
        newloc(pp, pl, pt, 0, 0 - loff);
        np = np + 1;
        if (!isp(',')) break;
        lex();
    }
    expect(')');
    if (isp(';')) { lex(); return; }        /* a prototype */
    if (np > 6) die("too many parameters");
    es(":F_");
    emitname(p, l);
    ech(10);
    es("55 48 89 E5 48 81 EC 00 10 00 00 ");
    i = 0;
    while (i < np) {
        if (i == 0) es("48 89 BD ");
        if (i == 1) es("48 89 B5 ");
        if (i == 2) es("48 89 95 ");
        if (i == 3) es("48 89 8D ");
        if (i == 4) es("4C 89 85 ");
        if (i == 5) es("4C 89 8D ");
        enb(0 - (i + 1) * 8, 4);
        i = i + 1;
    }
    brklbl = 0;
    cntlbl = 0;
    stmt();
    es("48 89 EC 5D C3 ");
}

/* ------------------------------ driver ---------------------------- */
void header()
{
    es("7F 45 4C 46 02 01 01 00 00 00 00 00 00 00 00 00\n");
    es("02 00 3E 00 01 00 00 00\n");
    es("$__s\n");
    es("40 00 00 00 00 00 00 00\n");
    es("00 00 00 00 00 00 00 00\n");
    es("00 00 00 00 40 00 38 00 01 00 40 00 00 00 00 00\n");
    es("01 00 00 00 07 00 00 00\n");
    es("00 00 00 00 00 00 00 00\n");
    es("00 00 40 00 00 00 00 00\n");
    es("00 00 40 00 00 00 00 00\n");
    es("~__e 00 00 00 00\n");
    es("00 00 00 10 00 00 00 00\n");
    es("00 10 00 00 00 00 00 00\n");
    es(":__s\n");
    es("48 8B 3C 24 48 8D 74 24 08 E8 @F_main 48 89 C7 B8 3C 00 00 00 0F 05\n");
}

void strings()
{
    long i;
    long j;
    i = 0;
    while (i < nstr) {
        es(":S");
        edec(i);
        ech(10);
        j = 0;
        while (j < slen[i]) {
            ehb(sbuf[sstart[i] + j]);
            j = j + 1;
        }
        ech(10);
        i = i + 1;
    }
}

int main(int argc, char **argv)
{
    long n;
    long t;
    t = 0;
    while (1) {
        n = sys_read(0, src + t, 1000000);
        if (n <= 0) break;
        t = t + n;
    }
    src[t] = 0;
    srcp = src;
    line = 1;
    outn = 0;
    header();
    lex();
    while (tk != 0) decl();
    strings();
    es(":__e\n");
    sys_write(1, out, outn);
    return 0;
}
