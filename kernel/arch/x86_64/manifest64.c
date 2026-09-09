/* 41I: manifest import — anahtar=deger satirlari + paket donusumu.
 * Desteklenen anahtarlar: name, runtime, sdk, branch, command.
 * runtime degeri flatpak ref biciminde olabilir ("runtime/.../.../..").
 */
#include "arch/x86_64/longmode.h"

static void man_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int man_key_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int manifest64_parse(const char *buf, struct manifest64 *out) {
    const char *p;
    if (!buf || !out) return -1;
    out->name[0] = 0;
    out->runtime[0] = 0;
    out->sdk[0] = 0;
    out->branch[0] = 0;
    out->command[0] = 0;
    p = buf;
    while (*p) {
        char key[32];
        char val[64];
        int ki = 0, vi = 0;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n') {
            while (*p && *p != '\n') p++;
            if (*p) p++;
            continue;
        }
        if (!*p) break;
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
        while (*p && *p != '\n' && vi < 63) val[vi++] = *p++;
        val[vi] = 0;
        while (vi > 0 && (val[vi - 1] == ' ' || val[vi - 1] == '\t' ||
                          val[vi - 1] == '\r'))
            val[--vi] = 0;
        if (*p) p++;
        if (man_key_eq(key, "name"))
            man_copy(out->name, val, 64);
        else if (man_key_eq(key, "runtime"))
            man_copy(out->runtime, val, 64);
        else if (man_key_eq(key, "sdk"))
            man_copy(out->sdk, val, 64);
        else if (man_key_eq(key, "branch"))
            man_copy(out->branch, val, 16);
        else if (man_key_eq(key, "command"))
            man_copy(out->command, val, 64);
    }
    if (!out->name[0]) return -2; /* ad sart */
    return 0;
}

/* Manifest -> repo kaydi: paket adi + calisma-zamani bagimliligi. */
int manifest64_to_pkg(const struct manifest64 *m) {
    char dep[48];
    int i = 0, j;
    const char *r;
    if (!m || !m->name[0]) return -1;
    if (repo64_add(m->name, m->branch[0] ? m->branch : "1.0", "flatpak") !=
        0)
        return -2;
    if (m->runtime[0]) {
        /* "runtime/org..Clang/x86_64/stable" -> bagimlilik adi */
        r = m->runtime;
        /* tur one Ekini atla */
        {
            const char *q = r;
            int sl = 0;
            while (*q) {
                if (*q == '/') sl++;
                q++;
            }
            if (sl == 3) {
                while (*r && *r != '/') r++;
                if (*r) r++;
            }
        }
        for (j = 0; r[j] && r[j] != '/' && i < 31; j++, i++)
            dep[i] = r[j];
        dep[i] = 0;
        {
            /* depsolve meta: paket -> runtime bagimliligi */
            char deps[1][48];
            for (j = 0; dep[j] && j < 47; j++) deps[0][j] = dep[j];
            deps[0][j] = 0;
            depsolve64_add_meta(m->name,
                                m->branch[0] ? m->branch : "1.0", deps, 1);
        }
    }
    return 0;
}
