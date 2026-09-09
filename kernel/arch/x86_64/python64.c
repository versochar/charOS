/* 54H: python runtime skeleton — nesne modeli + tamsayi ifade cozucu. */
#include "arch/x86_64/longmode.h"

int python64_int(long long v, struct python64_obj *out) {
    if (!out) return -1;
    out->type = PYTHON64_INT;
    out->ival = v;
    out->sval[0] = 0;
    return 0;
}

int python64_str(const char *s, struct python64_obj *out) {
    int i;
    if (!s || !out) return -1;
    out->type = PYTHON64_STR;
    out->ival = 0;
    for (i = 0; s[i] && i < 63; i++) out->sval[i] = s[i];
    out->sval[i] = 0;
    return 0;
}

int python64_add(const struct python64_obj *a,
                 const struct python64_obj *b, struct python64_obj *out) {
    int i, j;
    if (!a || !b || !out) return -1;
    if (a->type == PYTHON64_INT && b->type == PYTHON64_INT) {
        out->type = PYTHON64_INT;
        out->ival = a->ival + b->ival;
        out->sval[0] = 0;
        return 0;
    }
    if (a->type == PYTHON64_STR && b->type == PYTHON64_STR) {
        out->type = PYTHON64_STR;
        out->ival = 0;
        for (i = 0; a->sval[i] && i < 63; i++) out->sval[i] = a->sval[i];
        for (j = 0; b->sval[j] && i < 63; j++, i++)
            out->sval[i] = b->sval[j];
        out->sval[i] = 0;
        return 0;
    }
    return -2; /* tur uyusmazligi */
}

/* Ozyinelemeli inis: expr := term (('+'|'-') term)* ; term := faktor ... */
struct python64_parser {
    const char *p;
    int ok;
};

static void py_skip(struct python64_parser *x) {
    while (*x->p == ' ' || *x->p == '\t') x->p++;
}

static long long py_expr(struct python64_parser *x);

static long long py_factor(struct python64_parser *x) {
    long long v = 0;
    int neg = 0;
    py_skip(x);
    if (*x->p == '-') {
        neg = 1;
        x->p++;
        py_skip(x);
    } else if (*x->p == '+') {
        x->p++;
        py_skip(x);
    }
    if (*x->p == '(') {
        x->p++;
        v = py_expr(x);
        py_skip(x);
        if (*x->p == ')')
            x->p++;
        else
            x->ok = 0;
        return neg ? -v : v;
    }
    if (*x->p < '0' || *x->p > '9') {
        x->ok = 0;
        return 0;
    }
    while (*x->p >= '0' && *x->p <= '9') {
        v = v * 10 + (*x->p - '0');
        x->p++;
    }
    return neg ? -v : v;
}

static long long py_term(struct python64_parser *x) {
    long long v = py_factor(x);
    for (;;) {
        char op;
        long long r;
        py_skip(x);
        op = *x->p;
        if (op != '*' && op != '/' && op != '%') return v;
        x->p++;
        r = py_factor(x);
        if (!x->ok) return 0;
        if (op == '*') {
            v *= r;
        } else if (r == 0) {
            x->ok = 0;
            return 0;
        } else if (op == '/') {
            v /= r;
        } else {
            v %= r;
        }
    }
}

static long long py_expr(struct python64_parser *x) {
    long long v = py_term(x);
    for (;;) {
        char op;
        long long r;
        py_skip(x);
        op = *x->p;
        if (op != '+' && op != '-') return v;
        x->p++;
        r = py_term(x);
        if (!x->ok) return 0;
        v = (op == '+') ? v + r : v - r;
    }
}

long long python64_eval_int(const char *expr, int *ok) {
    struct python64_parser x;
    long long v;
    if (!expr) {
        if (ok) *ok = 0;
        return 0;
    }
    x.p = expr;
    x.ok = 1;
    v = py_expr(&x);
    py_skip(&x);
    if (*x.p) x.ok = 0; /* artan karakter */
    if (ok) *ok = x.ok;
    return x.ok ? v : 0;
}

int python64_print(const struct python64_obj *o, char *out, int max) {
    int i, pos = 0;
    char tmp[24];
    int n = 0;
    long long v;
    if (!o || !out || max <= 0) return -1;
    if (o->type == PYTHON64_STR) {
        for (i = 0; o->sval[i] && pos + 1 < max; i++)
            out[pos++] = o->sval[i];
        out[pos] = 0;
        return 0;
    }
    {
        unsigned long long u;
        v = o->ival;
        if (v < 0) {
            if (pos + 1 >= max) return -2;
            out[pos++] = '-';
            u = (unsigned long long)(-(v + 1)) + 1ULL;
            /* ... asagidaki dongu u ile calisir */
            if (!u)
                tmp[n++] = '0';
            while (u && n < 23) {
                tmp[n++] = (char)('0' + (u % 10));
                u /= 10;
            }
            while (n > 0 && pos + 1 < max) out[pos++] = tmp[--n];
            out[pos] = 0;
            return 0;
        }
    }
    v = o->ival;
    if (!v)
        tmp[n++] = '0';
    while (v && n < 23) {
        tmp[n++] = (char)('0' + (v % 10));
        v /= 10;
    }
    while (n > 0 && pos + 1 < max) out[pos++] = tmp[--n];
    out[pos] = 0;
    return 0;
}
