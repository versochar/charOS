/* 35D: OOM killer — skor = rss + adj agirligi; en kotu secilir, oldurulur. */
#include "arch/x86_64/longmode.h"

struct oom64_task {
    int used;
    int pid;
    u64 rss;
    int adj;
    int dead;
};

static struct oom64_task oom64_tab[OOM64_MAX_TASKS];

static long oom64_score(const struct oom64_task *t) {
    return (long)t->rss + (long)t->adj * 100L;
}

int oom64_add_task(int pid, u64 rss_pages, int adj) {
    int i;
    if (pid <= 0) return -1;
    for (i = 0; i < OOM64_MAX_TASKS; i++) {
        if (oom64_tab[i].used && oom64_tab[i].pid == pid) {
            oom64_tab[i].rss = rss_pages;
            oom64_tab[i].adj = adj;
            return 0;
        }
    }
    for (i = 0; i < OOM64_MAX_TASKS; i++) {
        if (!oom64_tab[i].used) {
            oom64_tab[i].used = 1;
            oom64_tab[i].pid = pid;
            oom64_tab[i].rss = rss_pages;
            oom64_tab[i].adj = adj;
            oom64_tab[i].dead = 0;
            return 0;
        }
    }
    return -2; /* dolu */
}

int oom64_pick(void) {
    int i, best = -1;
    long bestscore = -1;
    for (i = 0; i < OOM64_MAX_TASKS; i++) {
        if (!oom64_tab[i].used || oom64_tab[i].dead) continue;
        if (oom64_score(&oom64_tab[i]) > bestscore) {
            bestscore = oom64_score(&oom64_tab[i]);
            best = oom64_tab[i].pid;
        }
    }
    return best;
}

int oom64_kill(int pid) {
    int i;
    for (i = 0; i < OOM64_MAX_TASKS; i++) {
        if (oom64_tab[i].used && oom64_tab[i].pid == pid &&
            !oom64_tab[i].dead) {
            oom64_tab[i].dead = 1;
            oom64_tab[i].rss = 0;
            return 0;
        }
    }
    return -1;
}

int oom64_killed(int pid) {
    int i;
    for (i = 0; i < OOM64_MAX_TASKS; i++) {
        if (oom64_tab[i].used && oom64_tab[i].pid == pid)
            return oom64_tab[i].dead;
    }
    return 0;
}
