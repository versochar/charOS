/* 52H: symbol versioning — tanim/istek + surum karsilastirma. */
#include "arch/x86_64/longmode.h"

#define SYMVER64_MAX_DEF 64
#define SYMVER64_MAX_NEED 64

struct symver64_entry {
    int used;
    char name[48];
    char ver[16];
};

static struct symver64_entry symver64_defs[SYMVER64_MAX_DEF];
static struct symver64_entry symver64_needs[SYMVER64_MAX_NEED];

static void symver_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int symver_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* "LIBC_2.1" -> sayisal kuyruk karsilastirma (repo64_vercmp ustu). */
static int symver_cmp(const char *have, const char *want) {
    const char *h = have, *w = want;
    int i;
    /* Son '_' sonrasi surum */
    for (i = 0; have[i]; i++) {
        if (have[i] == '_') h = have + i + 1;
    }
    for (i = 0; want[i]; i++) {
        if (want[i] == '_') w = want + i + 1;
    }
    return repo64_vercmp(h, w);
}

int symver64_add_def(const char *name, const char *ver) {
    int i;
    if (!name || !ver) return -1;
    for (i = 0; i < SYMVER64_MAX_DEF; i++) {
        if (!symver64_defs[i].used) {
            symver64_defs[i].used = 1;
            symver_str_copy(symver64_defs[i].name, name, 48);
            symver_str_copy(symver64_defs[i].ver, ver, 16);
            return 0;
        }
    }
    return -2;
}

int symver64_need(const char *name, const char *ver) {
    int i;
    if (!name || !ver) return -1;
    for (i = 0; i < SYMVER64_MAX_NEED; i++) {
        if (!symver64_needs[i].used) {
            symver64_needs[i].used = 1;
            symver_str_copy(symver64_needs[i].name, name, 48);
            symver_str_copy(symver64_needs[i].ver, ver, 16);
            return 0;
        }
    }
    return -2;
}

/* 0=tumu saglandi, >0 eksik sayisi. */
int symver64_check(void) {
    int i, missing = 0;
    for (i = 0; i < SYMVER64_MAX_NEED; i++) {
        int j, ok = 0;
        if (!symver64_needs[i].used) continue;
        for (j = 0; j < SYMVER64_MAX_DEF; j++) {
            if (!symver64_defs[j].used) continue;
            if (!symver_str_eq(symver64_defs[j].name,
                               symver64_needs[i].name))
                continue;
            if (symver_cmp(symver64_defs[j].ver, symver64_needs[i].ver) >=
                0)
                ok = 1;
        }
        if (!ok) missing++;
    }
    return missing;
}
