/* 58D: PID namespace */
#include "arch/x86_64/longmode.h"

#define PIDNS_MAX 32

struct pidns_entry {
    int used;
    int id;
    int init_pid;
};

static struct pidns_entry pidns_tab[PIDNS_MAX];
static int pidns_next = 1;

int pidns64_create(int *out_id) {
    int i;
    if (!out_id) return -1;
    for (i = 0; i < PIDNS_MAX; i++) {
        if (!pidns_tab[i].used) {
            pidns_tab[i].used = 1;
            pidns_tab[i].id = pidns_next++;
            pidns_tab[i].init_pid = 1;
            *out_id = pidns_tab[i].id;
            return 0;
        }
    }
    return -2;
}

int pidns64_destroy(int id) {
    int i;
    for (i = 0; i < PIDNS_MAX; i++) {
        if (pidns_tab[i].used && pidns_tab[i].id == id) {
            pidns_tab[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int pidns64_count(int *out) {
    int i, cnt = 0;
    if (!out) return -1;
    for (i = 0; i < PIDNS_MAX; i++) if (pidns_tab[i].used) cnt++;
    *out = cnt;
    return 0;
}
