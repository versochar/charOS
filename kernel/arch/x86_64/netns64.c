/* 58E: network namespace */
#include "arch/x86_64/longmode.h"

#define NETNS_MAX 32

struct netns_entry {
    int used;
    int id;
    char veth[64];
};

static struct netns_entry netns_tab[NETNS_MAX];
static int netns_next = 1;

int netns64_create(int *out_id) {
    int i;
    if (!out_id) return -1;
    for (i = 0; i < NETNS_MAX; i++) {
        if (!netns_tab[i].used) {
            netns_tab[i].used = 1;
            netns_tab[i].id = netns_next++;
            netns_tab[i].veth[0] = 0;
            *out_id = netns_tab[i].id;
            return 0;
        }
    }
    return -2;
}

int netns64_destroy(int id) {
    int i;
    for (i = 0; i < NETNS_MAX; i++) {
        if (netns_tab[i].used && netns_tab[i].id == id) {
            netns_tab[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int netns64_count(int *out) {
    int i, cnt = 0;
    if (!out) return -1;
    for (i = 0; i < NETNS_MAX; i++) if (netns_tab[i].used) cnt++;
    *out = cnt;
    return 0;
}
