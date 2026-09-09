/* 41G: mirror — oncelikli saglikli ayna secimi + eszamanlama damgasi. */
#include "arch/x86_64/longmode.h"

#define MIRROR64_MAX 8

struct mirror64_entry {
    int used;
    char url[96];
    int prio;
    int healthy;
    u64 last_sync;
};

static struct mirror64_entry mirror64_tab[MIRROR64_MAX];

static void mir_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int mir_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int mirror64_add(const char *url, int prio) {
    int i;
    if (!url) return -1;
    for (i = 0; i < MIRROR64_MAX; i++) {
        if (mirror64_tab[i].used && mir_eq(mirror64_tab[i].url, url)) {
            mirror64_tab[i].prio = prio;
            mirror64_tab[i].healthy = 1;
            return 0;
        }
    }
    for (i = 0; i < MIRROR64_MAX; i++) {
        if (!mirror64_tab[i].used) {
            mirror64_tab[i].used = 1;
            mir_copy(mirror64_tab[i].url, url, 96);
            mirror64_tab[i].prio = prio;
            mirror64_tab[i].healthy = 1;
            mirror64_tab[i].last_sync = 0;
            return 0;
        }
    }
    return -2;
}

int mirror64_fail(const char *url) {
    int i;
    if (!url) return -1;
    for (i = 0; i < MIRROR64_MAX; i++) {
        if (mirror64_tab[i].used && mir_eq(mirror64_tab[i].url, url)) {
            mirror64_tab[i].healthy = 0;
            return 0;
        }
    }
    return -2;
}

int mirror64_sync_ok(const char *url, u64 now) {
    int i;
    if (!url) return -1;
    for (i = 0; i < MIRROR64_MAX; i++) {
        if (mirror64_tab[i].used && mir_eq(mirror64_tab[i].url, url)) {
            mirror64_tab[i].healthy = 1;
            mirror64_tab[i].last_sync = now;
            return 0;
        }
    }
    return -2;
}

/* En yuksek oncelikli saglikli ayna. */
int mirror64_pick(char *out, int max) {
    int i, best = -1;
    for (i = 0; i < MIRROR64_MAX; i++) {
        if (!mirror64_tab[i].used || !mirror64_tab[i].healthy) continue;
        if (best < 0 || mirror64_tab[i].prio > mirror64_tab[best].prio)
            best = i;
    }
    if (best < 0) return -1;
    if (out && max > 0) mir_copy(out, mirror64_tab[best].url, max);
    return 0;
}
