/* 39E: fcntl lock — aralik cakisma denetimi (paylasimli/ozel). */
#include "arch/x86_64/longmode.h"

#define FLOCK64_MAX 32

struct flock64_entry {
    int used;
    u64 ino;
    int owner;
    int type; /* SH/EX */
    u64 start;
    u64 len; /* 0 = sonsuz */
};

static struct flock64_entry flock64_tab[FLOCK64_MAX];

static int flock_overlap(u64 s1, u64 l1, u64 s2, u64 l2) {
    u64 e1 = l1 ? s1 + l1 : ~0ULL;
    u64 e2 = l2 ? s2 + l2 : ~0ULL;
    return s1 < e2 && s2 < e1;
}

int flock64_lock(u64 ino, int owner, int type, u64 start, u64 len) {
    int i;
    if ((type != FLOCK64_SH && type != FLOCK64_EX) || !ino) return -1;
    if (flock64_check(ino, owner, type, start, len) != 0) return -2;
    for (i = 0; i < FLOCK64_MAX; i++) {
        if (!flock64_tab[i].used) {
            flock64_tab[i].used = 1;
            flock64_tab[i].ino = ino;
            flock64_tab[i].owner = owner;
            flock64_tab[i].type = type;
            flock64_tab[i].start = start;
            flock64_tab[i].len = len;
            return 0;
        }
    }
    return -3;
}

int flock64_unlock(u64 ino, int owner) {
    int i, n = 0;
    for (i = 0; i < FLOCK64_MAX; i++) {
        if (flock64_tab[i].used && flock64_tab[i].ino == ino &&
            flock64_tab[i].owner == owner) {
            flock64_tab[i].used = 0;
            n++;
        }
    }
    return n ? 0 : -1;
}

/* 0=tutulabilir, <0 cakisma */
int flock64_check(u64 ino, int owner, int type, u64 start, u64 len) {
    int i;
    if ((type != FLOCK64_SH && type != FLOCK64_EX) || !ino) return -1;
    for (i = 0; i < FLOCK64_MAX; i++) {
        struct flock64_entry *e = &flock64_tab[i];
        if (!e->used || e->ino != ino || e->owner == owner) continue;
        if (!flock_overlap(e->start, e->len, start, len)) continue;
        if (e->type == FLOCK64_EX || type == FLOCK64_EX) return -2;
    }
    return 0;
}
