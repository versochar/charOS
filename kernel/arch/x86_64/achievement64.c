/* 57F: achievement sistemi — kazanma/ilgi/ilerleme. */
#include "arch/x86_64/longmode.h"

#define ACH_MAX 256

struct ach_entry {
    int used;
    int id;
    char name[64];
    char desc[128];
    int unlocked;
    int progress;
};

static struct ach_entry ach_tab[ACH_MAX];

static void ach_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct ach_entry *ach_find(int id) {
    int i;
    for (i = 0; i < ACH_MAX; i++) {
        if (ach_tab[i].used && ach_tab[i].id == id) return &ach_tab[i];
    }
    return 0;
}

int achievement64_add(int id, const char *name, const char *desc) {
    int i;
    if (id <= 0 || !name) return -1;
    if (ach_find(id)) return -2;
    for (i = 0; i < ACH_MAX; i++) {
        if (!ach_tab[i].used) {
            ach_tab[i].used = 1;
            ach_tab[i].id = id;
            ach_str_copy(ach_tab[i].name, name, 64);
            ach_str_copy(ach_tab[i].desc, desc ? desc : "", 128);
            ach_tab[i].unlocked = 0;
            ach_tab[i].progress = 0;
            return 0;
        }
    }
    return -3;
}

int achievement64_unlock(int id) {
    struct ach_entry *a = ach_find(id);
    if (!a) return -1;
    a->unlocked = 1;
    a->progress = 100;
    return 0;
}

int achievement64_is_unlocked(int id) {
    struct ach_entry *a = ach_find(id);
    if (!a) return -2;
    return a->unlocked ? 1 : 0;
}

int achievement64_progress(int id, int pct) {
    struct ach_entry *a = ach_find(id);
    if (!a) return -1;
    if (pct < 0 || pct > 100) return -2;
    a->progress = pct;
    if (pct >= 100) a->unlocked = 1;
    return 0;
}

int achievement64_get_progress(int id, int *out) {
    struct ach_entry *a = ach_find(id);
    if (!a || !out) return -1;
    *out = a->progress;
    return 0;
}

int achievement64_count(int *out) {
    int i, cnt = 0;
    if (!out) return -1;
    for (i = 0; i < ACH_MAX; i++) if (ach_tab[i].used) cnt++;
    *out = cnt;
    return 0;
}

int achievement64_list_ids(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < ACH_MAX && n < max; i++) {
        if (ach_tab[i].used) ids[n++] = ach_tab[i].id;
    }
    return n;
}
