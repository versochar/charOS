/* 51E: locale — ayirici/para-birimi + binlik gruplama. */
#include "arch/x86_64/longmode.h"

#define LOCALE64_MAX 8

struct locale64_entry {
    int used;
    char name[32];
    char decimal;
    char thousands;
    char currency[8];
};

static struct locale64_entry locale64_tab[LOCALE64_MAX];
static int locale64_cur = -1;

static void locale_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int locale_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static void locale64_builtin(void) {
    static int done = 0;
    int i;
    if (done) return;
    done = 1;
    i = 0;
    locale64_tab[i].used = 1;
    locale_str_copy(locale64_tab[i].name, "C", 32);
    locale64_tab[i].decimal = '.';
    locale64_tab[i].thousands = 0;
    locale64_tab[i].currency[0] = 0;
    i = 1;
    locale64_tab[i].used = 1;
    locale_str_copy(locale64_tab[i].name, "tr_TR", 32);
    locale64_tab[i].decimal = ',';
    locale64_tab[i].thousands = '.';
    locale_str_copy(locale64_tab[i].currency, "TL", 8);
}

int locale64_set(const char *name) {
    int i;
    locale64_builtin();
    if (!name) return -1;
    for (i = 0; i < LOCALE64_MAX; i++) {
        if (locale64_tab[i].used &&
            locale_str_eq(locale64_tab[i].name, name)) {
            locale64_cur = i;
            return 0;
        }
    }
    return -2;
}

int locale64_conv(char *dec, char *thousands, char *currency, int max) {
    struct locale64_entry *e;
    int i;
    if (locale64_cur < 0) locale64_builtin();
    if (locale64_cur < 0) return -1;
    e = &locale64_tab[locale64_cur];
    if (dec && max > 1) {
        dec[0] = e->decimal;
        dec[1] = 0;
    }
    if (thousands && max > 1) {
        thousands[0] = e->thousands ? e->thousands : 0;
        thousands[1] = 0;
    }
    if (currency && max > 0) {
        for (i = 0; e->currency[i] && i + 1 < max; i++)
            currency[i] = e->currency[i];
        currency[i] = 0;
    }
    return 0;
}

int locale64_format_num(long long v, char *out, int max) {
    struct locale64_entry *e;
    char digits[32];
    int nd = 0, pos = 0, neg = 0, i, cnt = 0;
    unsigned long long u;
    if (!out || max <= 0) return -1;
    if (locale64_cur < 0) locale64_builtin();
    e = locale64_cur >= 0 ? &locale64_tab[locale64_cur]
                          : &locale64_tab[0];
    if (v < 0) {
        neg = 1;
        u = (unsigned long long)(-(v + 1)) + 1ULL;
    } else {
        u = (unsigned long long)v;
    }
    if (!u)
        digits[nd++] = '0';
    while (u && nd < 31) {
        digits[nd++] = (char)('0' + (u % 10));
        u /= 10;
    }
    /* Ters yaz + binlik ayirici */
    if (neg) {
        if (pos + 1 >= max) return -2;
        out[pos++] = '-';
    }
    for (i = nd - 1; i >= 0; i--) {
        if (pos + 2 >= max) return -2;
        out[pos++] = digits[i];
        cnt++;
        /* Ayirici sagdan her 3 basamakta: kalan i basamak 3'un katiysa */
        if (e->thousands && i > 0 && i % 3 == 0) out[pos++] = e->thousands;
    }
    out[pos] = 0;
    return 0;
}
