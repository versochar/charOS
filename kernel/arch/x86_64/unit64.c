/* 42B: unit parser — [Unit]/[Service] bolumlu INI.
 * Anahtarlar: Description, ExecStart, After, Wants (virgullu liste).
 */
#include "arch/x86_64/longmode.h"

static void unit_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int unit_key_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* Virgullu listeyi hedef diziye doldurur. */
static void unit_split_list(const char *v, char dst[][32], int *n, int max) {
    *n = 0;
    while (*v && *n < max) {
        int i = 0;
        while (*v == ' ' || *v == '\t') v++;
        if (!*v) break;
        while (*v && *v != ',' && *v != '\n' && i < 31)
            dst[*n][i++] = *v++;
        dst[*n][i] = 0;
        /* Sondaki bosluklari kirp */
        while (i > 0 && (dst[*n][i - 1] == ' ' || dst[*n][i - 1] == '\t'))
            dst[*n][--i] = 0;
        if (i) (*n)++;
        if (*v == ',') v++;
    }
}

int unit64_parse(const char *buf, struct unit64 *out) {
    const char *p;
    int section = 0; /* 1=Unit, 2=Service */
    if (!buf || !out) return -1;
    out->name[0] = 0;
    out->desc[0] = 0;
    out->exec[0] = 0;
    out->nafter = 0;
    out->nwants = 0;
    p = buf;
    while (*p) {
        char key[32], val[96];
        int ki = 0, vi = 0;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == ';' || *p == '\n') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        if (!*p) break;
        if (*p == '[') {
            p++;
            if (*p == 'U') section = 1;
            else if (*p == 'S') section = 2;
            else section = 0;
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        while (*p && *p != '=' && *p != '\n' && ki < 31)
            key[ki++] = *p++;
        key[ki] = 0;
        if (*p != '=') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        p++;
        while (*p == ' ' || *p == '\t') p++;
        while (*p && *p != '\n' && vi < 95) val[vi++] = *p++;
        val[vi] = 0;
        while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t' ||
                          val[vi - 1] == '\r'))
            val[--vi] = 0;
        if (*p) p++;
        if (section == 1) {
            if (unit_key_eq(key, "Description"))
                unit_copy(out->desc, val, 64);
            else if (unit_key_eq(key, "After"))
                unit_split_list(val, out->after, &out->nafter, 8);
            else if (unit_key_eq(key, "Wants"))
                unit_split_list(val, out->wants, &out->nwants, 8);
        } else if (section == 2) {
            if (unit_key_eq(key, "ExecStart"))
                unit_copy(out->exec, val, 96);
        }
    }
    if (!out->exec[0]) return -2; /* calistirilabilir sart */
    return 0;
}
