/* 40F: kota — kullanici basina blok/inode sayac + limit. */
#include "arch/x86_64/longmode.h"

#define QUOTA64_MAX 32

struct quota64_entry {
    int used;
    u32 uid;
    u64 blocks;
    u64 inodes;
    u64 blim; /* 0 = sinirsiz */
    u64 ilim;
};

static struct quota64_entry quota64_tab[QUOTA64_MAX];

static struct quota64_entry *quota64_get(u32 uid) {
    int i;
    for (i = 0; i < QUOTA64_MAX; i++)
        if (quota64_tab[i].used && quota64_tab[i].uid == uid)
            return &quota64_tab[i];
    return 0;
}

int quota64_set_limit(u32 uid, u64 blim, u64 ilim) {
    struct quota64_entry *e = quota64_get(uid);
    int i;
    if (e) {
        e->blim = blim;
        e->ilim = ilim;
        return 0;
    }
    for (i = 0; i < QUOTA64_MAX; i++) {
        if (!quota64_tab[i].used) {
            quota64_tab[i].used = 1;
            quota64_tab[i].uid = uid;
            quota64_tab[i].blocks = 0;
            quota64_tab[i].inodes = 0;
            quota64_tab[i].blim = blim;
            quota64_tab[i].ilim = ilim;
            return 0;
        }
    }
    return -1;
}

int quota64_charge(u32 uid, u64 blocks, u64 inodes) {
    struct quota64_entry *e = quota64_get(uid);
    if (!e) {
        if (quota64_set_limit(uid, 0, 0) != 0) return -1;
        e = quota64_get(uid);
    }
    if (e->blim && e->blocks + blocks > e->blim) return -2;
    if (e->ilim && e->inodes + inodes > e->ilim) return -3;
    e->blocks += blocks;
    e->inodes += inodes;
    return 0;
}

int quota64_release(u32 uid, u64 blocks, u64 inodes) {
    struct quota64_entry *e = quota64_get(uid);
    if (!e) return -1;
    e->blocks = (e->blocks >= blocks) ? e->blocks - blocks : 0;
    e->inodes = (e->inodes >= inodes) ? e->inodes - inodes : 0;
    return 0;
}

int quota64_check(u32 uid) {
    struct quota64_entry *e = quota64_get(uid);
    if (!e) return 0;
    if (e->blim && e->blocks > e->blim) return -1;
    if (e->ilim && e->inodes > e->ilim) return -2;
    return 0;
}
