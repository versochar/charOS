#include "arch/x86_64/longmode.h"
#include <string.h>

#define REPO64_MAX_ENTRIES 128
#define REPO64_VER_MAX      32
#define REPO64_REPO_MAX     48

struct repo_entry {
    char name[REPO64_REPO_MAX];
    char ver[REPO64_VER_MAX];
    char repo[REPO64_REPO_MAX];
    u8   enabled;
};
static struct repo_entry repo_tab[REPO64_MAX_ENTRIES];
static int repo_n = 0;

int repo64_add(const char *name, const char *ver, const char *repo) {
    int i;
    if (!name || !ver || !repo) return -1;
    /* birebir ayni kayit tekrari reddet; farkli surumler indexte durur */
    for (i = 0; i < repo_n; i++) {
        if (repo_tab[i].enabled && strcmp(repo_tab[i].repo, repo) == 0 &&
            strcmp(repo_tab[i].name, name) == 0 &&
            strcmp(repo_tab[i].ver, ver) == 0)
            return -2;
    }
    if (repo_n >= REPO64_MAX_ENTRIES) return -3;
    memset(&repo_tab[repo_n], 0, sizeof(repo_tab[repo_n]));
    strncpy(repo_tab[repo_n].name, name, sizeof(repo_tab[repo_n].name) - 1);
    strncpy(repo_tab[repo_n].ver, ver, sizeof(repo_tab[repo_n].ver) - 1);
    strncpy(repo_tab[repo_n].repo, repo, sizeof(repo_tab[repo_n].repo) - 1);
    repo_tab[repo_n].enabled = 1;
    repo_n++;
    return 0;
}

int repo64_find(const char *name, const char *ver_req, char *ver_out,
                int max) {
    int i, found = -1;
    if (!name || !ver_out || max <= 0) return -1;
    for (i = 0; i < repo_n; i++) {
        if (!repo_tab[i].enabled || strcmp(repo_tab[i].name, name) != 0)
            continue;
        /* surum filtresi: ver_req bos veya NULL ise herhangi biri */
        if (ver_req && ver_req[0] && repo64_vercmp(repo_tab[i].ver, ver_req) < 0)
            continue;
        if (found < 0 || repo64_vercmp(repo_tab[i].ver,
                                       repo_tab[found].ver) > 0)
            found = i;
    }
    if (found < 0) return -2;
    strncpy(ver_out, repo_tab[found].ver, (size_t)max - 1);
    ver_out[max - 1] = '\0';
    return 0;
}

int repo64_vercmp(const char *a, const char *b) {
    const char *pa = a, *pb = b;
    int na = 0, nb = 0;
    if (!a || !b) return 0;
    while (*pa && *pb) {
        if (*pa >= '0' && *pa <= '9' && *pb >= '0' && *pb <= '9') {
            int va = 0, vb = 0;
            while (*pa >= '0' && *pa <= '9') { va = va * 10 + (*pa - '0'); pa++; }
            while (*pb >= '0' && *pb <= '9') { vb = vb * 10 + (*pb - '0'); pb++; }
            if (va != vb) return va < vb ? -1 : 1;
        } else if (*pa != *pb) {
            return *pa < *pb ? -1 : 1;
        } else { pa++; pb++; }
    }
    if (*pa == '.') na++;
    if (*pb == '.') nb++;
    if (*pa) na += *pa;
    if (*pb) nb += *pb;
    return na < nb ? -1 : (na > nb ? 1 : 0);
}

int repo64_count(void) {
    return repo_n;
}