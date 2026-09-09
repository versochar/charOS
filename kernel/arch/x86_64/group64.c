/* 38F: group policy — grup/uye/kaynak kurallari. */
#include "arch/x86_64/longmode.h"

#define GROUP64_MAX 32
#define GROUP64_MAX_MEMBERS 32
#define GROUP64_MAX_POLICY 64

struct group64_entry {
    int used;
    char name[32];
    u32 gid;
    char members[GROUP64_MAX_MEMBERS][32];
    int nmembers;
};

struct group64_policy {
    int used;
    char group[32];
    char resource[64];
    int perm;
};

static struct group64_entry group64_tab[GROUP64_MAX];
static struct group64_policy group64_pol[GROUP64_MAX_POLICY];

static void grp_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int grp_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int group64_add(const char *name, u32 gid) {
    int i;
    if (!name) return -1;
    for (i = 0; i < GROUP64_MAX; i++) {
        if (group64_tab[i].used && grp_str_eq(group64_tab[i].name, name)) {
            group64_tab[i].gid = gid;
            return 0;
        }
    }
    for (i = 0; i < GROUP64_MAX; i++) {
        if (!group64_tab[i].used) {
            group64_tab[i].used = 1;
            grp_str_copy(group64_tab[i].name, name, 32);
            group64_tab[i].gid = gid;
            group64_tab[i].nmembers = 0;
            return 0;
        }
    }
    return -2;
}

int group64_add_member(const char *name, const char *user) {
    int i;
    if (!name || !user) return -1;
    for (i = 0; i < GROUP64_MAX; i++) {
        struct group64_entry *g = &group64_tab[i];
        int m;
        if (!g->used || !grp_str_eq(g->name, name)) continue;
        for (m = 0; m < g->nmembers; m++)
            if (grp_str_eq(g->members[m], user)) return 0;
        if (g->nmembers >= GROUP64_MAX_MEMBERS) return -2;
        grp_str_copy(g->members[g->nmembers++], user, 32);
        return 0;
    }
    return -3; /* grup yok */
}

int group64_is_member(const char *name, const char *user) {
    int i, m;
    if (!name || !user) return 0;
    for (i = 0; i < GROUP64_MAX; i++) {
        struct group64_entry *g = &group64_tab[i];
        if (!g->used || !grp_str_eq(g->name, name)) continue;
        for (m = 0; m < g->nmembers; m++)
            if (grp_str_eq(g->members[m], user)) return 1;
        return 0;
    }
    return 0;
}

int group64_policy_add(const char *group, const char *resource, int perm) {
    int i;
    if (!group || !resource) return -1;
    for (i = 0; i < GROUP64_MAX_POLICY; i++) {
        if (!group64_pol[i].used) {
            group64_pol[i].used = 1;
            grp_str_copy(group64_pol[i].group, group, 32);
            grp_str_copy(group64_pol[i].resource, resource, 64);
            group64_pol[i].perm = perm;
            return 0;
        }
    }
    return -2;
}

/* 1=serbest: kullanicinin HERHANGI grubu kaynaga bu izni veriyorsa. */
int group64_check(const char *user, const char *resource, int perm) {
    int i;
    if (!user || !resource) return 0;
    for (i = 0; i < GROUP64_MAX_POLICY; i++) {
        struct group64_policy *p = &group64_pol[i];
        if (!p->used) continue;
        if (!grp_str_eq(p->resource, resource)) continue;
        if (!(p->perm & perm)) continue;
        if (group64_is_member(p->group, user)) return 1;
    }
    return 0;
}
