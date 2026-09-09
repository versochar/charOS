/* 51H: regex — ozyinelemeli geri-izlemeli kucuk alt-kume.
 * Destek: literal, '.', '*', '+', '?', '[...]' (aralik + degilleme),
 * '^', '$'. Derinlik sinirli (yigin tasma emniyeti).
 */
#include "arch/x86_64/longmode.h"

#define REGEX64_MAX 16
#define REGEX64_DEPTH 64

static char regex64_pat[REGEX64_MAX][128];
static int regex64_used[REGEX64_MAX];

/* Sinif ici eslesme; *pi ']' sonrasina ilerler. */
static int regex_class(const char *pat, int *pi, char c) {
    int neg = 0, hit = 0;
    (*pi)++; /* '[' atla */
    if (pat[*pi] == '^') {
        neg = 1;
        (*pi)++;
    }
    while (pat[*pi] && pat[*pi] != ']') {
        if (pat[*pi + 1] == '-' && pat[*pi + 2] && pat[*pi + 2] != ']') {
            if (c >= pat[*pi] && c <= pat[*pi + 2]) hit = 1;
            *pi += 3;
        } else {
            if (c == pat[*pi]) hit = 1;
            (*pi)++;
        }
    }
    if (pat[*pi] == ']') (*pi)++;
    return neg ? !hit : hit;
}

/* Atom coz: pat[pi]'deki tek oge c ile eslesir mi? atom_end oge sonu. */
static int regex_atom(const char *pat, int pi, char c, int *atom_end) {
    if (pat[pi] == '[') {
        int q = pi, hit = 0;
        if (c) {
            int qq = pi;
            hit = regex_class(pat, &qq, c);
        }
        /* Sinif sonunu bul (tuketmeden) */
        q = pi + 1;
        if (pat[q] == '^') q++;
        while (pat[q] && pat[q] != ']') {
            if (pat[q + 1] == '-' && pat[q + 2] && pat[q + 2] != ']')
                q += 3;
            else
                q++;
        }
        if (pat[q] == ']') q++;
        *atom_end = q;
        return hit;
    }
    if (pat[pi] == '.') {
        *atom_end = pi + 1;
        return c ? 1 : 0;
    }
    if (pat[pi] == '\\' && pat[pi + 1]) {
        *atom_end = pi + 2;
        return c == pat[pi + 1];
    }
    *atom_end = pi + 1;
    return c == pat[pi];
}

/* pat[pi..] s[pos..end] ile TAM eslesir mi? */
static int regex_full(const char *pat, int pi, const char *s, int pos,
                      int end, int depth) {
    int ae = pi, m, op, n, i;
    if (depth > REGEX64_DEPTH) return 0;
    if (!pat[pi]) return pos == end;
    if (pat[pi] == '$' && !pat[pi + 1]) return pos == end;
    m = regex_atom(pat, pi, pos < end ? s[pos] : 0, &ae);
    (void)m;
    op = pat[ae];
    if (op == '*' || op == '+' || op == '?') {
        int lo = (op == '+') ? 1 : 0;
        int hi;
        n = 0;
        while (pos + n < end) {
            int e2;
            if (!regex_atom(pat, pi, s[pos + n], &e2) || n >= 256) break;
            n++;
        }
        hi = (op == '?') ? 1 : n; /* '?' en fazla 1 tuketir */
        if (hi > n) hi = n;
        for (i = hi; i >= lo; i--) {
            if (regex_full(pat, ae + 1, s, pos + i, end, depth + 1))
                return 1;
        }
        return 0;
    }
    if (pos >= end) return 0;
    {
        int e2;
        if (!regex_atom(pat, pi, s[pos], &e2)) return 0;
    }
    return regex_full(pat, ae, s, pos + 1, end, depth + 1);
}

int regex64_compile(const char *pat) {
    int i, n = 0;
    if (!pat) return -1;
    while (pat[n] && n < 127) n++;
    if (pat[n]) return -2;
    for (i = 0; i < REGEX64_MAX; i++) {
        int j;
        if (!regex64_used[i]) {
            regex64_used[i] = 1;
            for (j = 0; j <= n; j++) regex64_pat[i][j] = pat[j];
            return i;
        }
    }
    return -3;
}

/* Tam eslesme (tum dizgi). */
int regex64_match(int re, const char *s) {
    int len = 0, pi = 0;
    if (re < 0 || re >= REGEX64_MAX || !regex64_used[re] || !s)
        return -1;
    if (regex64_pat[re][0] == '^') pi = 1;
    while (s[len]) len++;
    return regex_full(regex64_pat[re], pi, s, 0, len, 0) ? 0 : -2;
}

/* Alt-dizgi arama; baslangic konumu dondurur. */
int regex64_search(int re, const char *s, int *start_out) {
    int len = 0, st, en, pi;
    if (re < 0 || re >= REGEX64_MAX || !regex64_used[re] || !s)
        return -1;
    while (s[len]) len++;
    pi = (regex64_pat[re][0] == '^') ? 1 : 0;
    for (st = 0; st <= len; st++) {
        if (pi && st > 0) break; /* ^ yalniz bas */
        for (en = st; en <= len; en++) {
            if (regex_full(regex64_pat[re], pi, s, st, en, 0)) {
                if (start_out) *start_out = st;
                return 0;
            }
        }
    }
    return -2;
}
