/* 57H: cloud save — bulut kaydet/yukle/senkron. */
#include "arch/x86_64/longmode.h"

#define CLOUD_MAX 32

struct cloud_entry {
    int used;
    char game[64];
    int slot;
    char data[256];
    int synced;
};

static struct cloud_entry cloud_tab[CLOUD_MAX];

static void cloud_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct cloud_entry *cloud_find(const char *game, int slot) {
    int i;
    for (i = 0; i < CLOUD_MAX; i++) {
        if (cloud_tab[i].used && str_eq(cloud_tab[i].game, game) && cloud_tab[i].slot == slot) {
            return &cloud_tab[i];
        }
    }
    return 0;
}

int cloud64_save(const char *game, int slot, const char *data) {
    struct cloud_entry *e = cloud_find(game, slot);
    int i;
    if (!game || !data) return -1;
    if (e) {
        cloud_str_copy(e->data, data, 256);
        e->synced = 0;
        return 0;
    }
    for (i = 0; i < CLOUD_MAX; i++) {
        if (!cloud_tab[i].used) {
            cloud_tab[i].used = 1;
            cloud_str_copy(cloud_tab[i].game, game, 64);
            cloud_tab[i].slot = slot;
            cloud_str_copy(cloud_tab[i].data, data, 256);
            cloud_tab[i].synced = 0;
            return 0;
        }
    }
    return -2;
}

int cloud64_load(const char *game, int slot, char *out, int max) {
    struct cloud_entry *e = cloud_find(game, slot);
    if (!e || !out) return -1;
    cloud_str_copy(out, e->data, max);
    return 0;
}

int cloud64_sync(const char *game, int slot) {
    struct cloud_entry *e = cloud_find(game, slot);
    if (!e) return -1;
    e->synced = 1;
    return 0;
}

int cloud64_status(const char *game, int slot, int *out) {
    struct cloud_entry *e = cloud_find(game, slot);
    if (!e || !out) return -1;
    *out = e->synced;
    return 0;
}

int cloud64_delete(const char *game, int slot) {
    struct cloud_entry *e = cloud_find(game, slot);
    if (!e) return -1;
    e->used = 0;
    return 0;
}


