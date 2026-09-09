/* 57G: mod destek — yukle/ayikla/aktif. */
#include "arch/x86_64/longmode.h"

#define MOD_MAX 64

struct mod_entry {
    int used;
    int id;
    char name[64];
    char path[128];
    int enabled;
    int verified;
};

static struct mod_entry mod_tab[MOD_MAX];
static int mod_next_id = 1;

static void mod_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static struct mod_entry *mod_find_by_id(int id) {
    int i;
    for (i = 0; i < MOD_MAX; i++) {
        if (mod_tab[i].used && mod_tab[i].id == id) return &mod_tab[i];
    }
    return 0;
}

int mod64_load(const char *name, const char *path) {
    int i;
    if (!name || !path) return -1;
    for (i = 0; i < MOD_MAX; i++) {
        if (mod_tab[i].used && mod_tab[i].id == mod_next_id) break;
    }
    for (i = 0; i < MOD_MAX; i++) {
        if (!mod_tab[i].used) {
            mod_tab[i].used = 1;
            mod_tab[i].id = mod_next_id++;
            mod_str_copy(mod_tab[i].name, name, 64);
            mod_str_copy(mod_tab[i].path, path, 128);
            mod_tab[i].enabled = 0;
            mod_tab[i].verified = 0;
            return mod_tab[i].id;
        }
    }
    return -2;
}

int mod64_unload(int id) {
    struct mod_entry *m = mod_find_by_id(id);
    if (!m) return -1;
    m->used = 0;
    return 0;
}

int mod64_enable(int id) {
    struct mod_entry *m = mod_find_by_id(id);
    if (!m) return -1;
    m->enabled = 1;
    return 0;
}

int mod64_disable(int id) {
    struct mod_entry *m = mod_find_by_id(id);
    if (!m) return -1;
    m->enabled = 0;
    return 0;
}

int mod64_verify(int id) {
    struct mod_entry *m = mod_find_by_id(id);
    if (!m) return -1;
    m->verified = 1;
    return 0;
}

int mod64_is_enabled(int id) {
    struct mod_entry *m = mod_find_by_id(id);
    if (!m) return -2;
    return m->enabled ? 1 : 0;
}

int mod64_count(int *out) {
    int i, cnt = 0;
    if (!out) return -1;
    for (i = 0; i < MOD_MAX; i++) if (mod_tab[i].used) cnt++;
    *out = cnt;
    return 0;
}

int mod64_list_ids(int *ids, int max) {
    int i, n = 0;
    if (!ids || max <= 0) return -1;
    for (i = 0; i < MOD_MAX && n < max; i++) {
        if (mod_tab[i].used) ids[n++] = mod_tab[i].id;
    }
    return n;
}
