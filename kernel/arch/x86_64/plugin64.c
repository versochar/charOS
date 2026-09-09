/* 53F: plugin API — yukleme/cagri + bagimlilik denetimi. */
#include "arch/x86_64/longmode.h"

#define PLUGIN64_MAX 16
#define PLUGIN64_MAX_DEPS 8

struct plugin64_entry {
    int used;
    char name[48];
    char ver[16];
    plugin64_entry entry;
    char deps[PLUGIN64_MAX_DEPS][48];
    int ndeps;
};

static struct plugin64_entry plugin64_tab[PLUGIN64_MAX];

static void plugin_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int plugin_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct plugin64_entry *plugin64_find(const char *name) {
    int i;
    for (i = 0; i < PLUGIN64_MAX; i++)
        if (plugin64_tab[i].used && plugin_str_eq(plugin64_tab[i].name, name))
            return &plugin64_tab[i];
    return 0;
}

int plugin64_load(const char *name, const char *ver, plugin64_entry entry) {
    struct plugin64_entry *e = plugin64_find(name);
    int i;
    if (!name || !entry) return -1;
    if (e) {
        if (ver) plugin_str_copy(e->ver, ver, 16);
        e->entry = entry;
        return e->entry(0, 0); /* init op=0 */
    }
    for (i = 0; i < PLUGIN64_MAX; i++) {
        if (!plugin64_tab[i].used) {
            plugin64_tab[i].used = 1;
            plugin_str_copy(plugin64_tab[i].name, name, 48);
            plugin_str_copy(plugin64_tab[i].ver, ver ? ver : "", 16);
            plugin64_tab[i].entry = entry;
            plugin64_tab[i].ndeps = 0;
            return plugin64_tab[i].entry(0, 0);
        }
    }
    return -2;
}

int plugin64_unload(const char *name) {
    struct plugin64_entry *e = plugin64_find(name);
    int rc;
    if (!e) return -1;
    rc = e->entry(1, 0); /* fini op=1 */
    e->used = 0;
    e->ndeps = 0;
    return rc;
}

int plugin64_call(const char *name, int op, u64 arg) {
    struct plugin64_entry *e = plugin64_find(name);
    if (!e) return -1;
    return e->entry(op, arg);
}

int plugin64_add_dep(const char *plugin, const char *dep) {
    struct plugin64_entry *e = plugin64_find(plugin);
    if (!e || !dep) return -1;
    if (e->ndeps >= PLUGIN64_MAX_DEPS) return -2;
    plugin_str_copy(e->deps[e->ndeps++], dep, 48);
    return 0;
}

int plugin64_check_deps(const char *plugin) {
    struct plugin64_entry *e = plugin64_find(plugin);
    int i;
    if (!e) return -1;
    for (i = 0; i < e->ndeps; i++) {
        if (!plugin64_find(e->deps[i])) return i + 1; /* eksik index+1 */
    }
    return 0;
}
