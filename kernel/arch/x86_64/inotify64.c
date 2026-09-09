/* 39F: inotify skeleton — izleme + olay kuyrugu. */
#include "arch/x86_64/longmode.h"

#define INOTIFY64_MAX_WD 16
#define INOTIFY64_MAX_EV 64

struct inotify64_ev {
    int wd;
    u64 ino;
    u32 mask;
};

static u64 inotify64_watch[INOTIFY64_MAX_WD];
static u32 inotify64_mask[INOTIFY64_MAX_WD];
static int inotify64_nwd = 0;
static struct inotify64_ev inotify64_q[INOTIFY64_MAX_EV];
static int inotify64_head = 0;
static int inotify64_n = 0;

int inotify64_add_watch(u64 ino, u32 mask) {
    int i;
    if (!ino) return -1;
    for (i = 0; i < inotify64_nwd; i++) {
        if (inotify64_watch[i] == ino) {
            inotify64_mask[i] |= mask;
            return i;
        }
    }
    if (inotify64_nwd >= INOTIFY64_MAX_WD) return -2;
    inotify64_watch[inotify64_nwd] = ino;
    inotify64_mask[inotify64_nwd] = mask;
    return inotify64_nwd++;
}

int inotify64_event(u64 ino, u32 mask) {
    int i, n = 0;
    for (i = 0; i < inotify64_nwd; i++) {
        u32 hit;
        if (inotify64_watch[i] != ino) continue;
        hit = inotify64_mask[i] & mask;
        if (!hit) continue;
        if (inotify64_n >= INOTIFY64_MAX_EV) return -2;
        inotify64_q[(inotify64_head + inotify64_n) % INOTIFY64_MAX_EV].wd =
            i;
        inotify64_q[(inotify64_head + inotify64_n) % INOTIFY64_MAX_EV].ino =
            ino;
        inotify64_q[(inotify64_head + inotify64_n) % INOTIFY64_MAX_EV].mask =
            hit;
        inotify64_n++;
        n++;
    }
    return n;
}

int inotify64_read(int wd, u64 *ino, u32 *mask) {
    int i;
    for (i = 0; i < inotify64_n; i++) {
        int idx = (inotify64_head + i) % INOTIFY64_MAX_EV;
        if (inotify64_q[idx].wd != wd) continue;
        if (ino) *ino = inotify64_q[idx].ino;
        if (mask) *mask = inotify64_q[idx].mask;
        /* Kuyruktan cikar (kaydir) */
        for (; i + 1 < inotify64_n; i++) {
            int a = (inotify64_head + i) % INOTIFY64_MAX_EV;
            int b = (inotify64_head + i + 1) % INOTIFY64_MAX_EV;
            inotify64_q[a] = inotify64_q[b];
        }
        inotify64_n--;
        return 0;
    }
    return -1; /* bu wd'ye olay yok */
}
