#include "arch/x86_64/update.h"
#include <string.h>

struct upd_pkg {
    char name[UPDATE64_VER_MAX];
    char ver[UPDATE64_VER_MAX];
    u32  size;
    u8   applied;
};
static struct upd_pkg upd_tab[UPDATE64_MAX_PKGS];
static int upd_n = 0;
static int upd_state = UPDATE_OK;

static int upd_ver_cmp(const char *a, const char *b) {
    int na = 0, nb = 0;
    const char *pa = a, *pb = b;
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
    /* denormal (missing dot) esittir */
    if (!*pa && *pb == '.') { pb++; na++; }
    if (!*pb && *pa == '.') { pa++; nb++; }
    if (*pa) na++;
    if (*pb) nb++;
    return na < nb ? -1 : (na > nb ? 1 : 0);
}

int update64_init(void) {
    upd_n = 0;
    upd_state = UPDATE_OK;
    return 0;
}

int update64_stage(const char *name, const char *ver, u32 size_bytes) {
    struct upd_pkg *p;
    if (!name || !ver) return -1;
    if (upd_n >= UPDATE64_MAX_PKGS) return -2;
    if (size_bytes == 0 || size_bytes > 0x40000000u) return -3;
    if (upd_state != UPDATE_OK && upd_state != UPDATE_STAGED) return -4;
    p = &upd_tab[upd_n];
    memset(p, 0, sizeof(*p));
    strncpy(p->name, name, sizeof(p->name) - 1);
    strncpy(p->ver, ver, sizeof(p->ver) - 1);
    p->size = size_bytes;
    upd_n++;
    upd_state = UPDATE_STAGED;
    return 0;
}

int update64_check_compat(void) {
    int i, j;
    if (upd_n == 0) return -1;
    /* ayni paket iki kez farkli surumle silahliysa kabul etme */
    for (i = 0; i < upd_n; i++) {
        for (j = i + 1; j < upd_n; j++) {
            if (strcmp(upd_tab[i].name, upd_tab[j].name) == 0 &&
                upd_ver_cmp(upd_tab[i].ver, upd_tab[j].ver) != 0)
                return -2;
        }
    }
    return 0;
}

int update64_apply(void) {
    int i;
    if (upd_n == 0) return -1;
    if (update64_check_compat() != 0) { upd_state = UPDATE_FAILED; return -2; }
    upd_state = UPDATE_APPLYING;
    for (i = 0; i < upd_n; i++)
        upd_tab[i].applied = 1;
    upd_state = UPDATE_STAGED;
    return 0;
}

int update64_verify(void) {
    int i;
    if (upd_n == 0) return -1;
    for (i = 0; i < upd_n; i++)
        if (upd_tab[i].size > 0x40000000u || upd_tab[i].size == 0)
            return -1;
    return 0;
}

int update64_rollback(void) {
    int i;
    if (upd_state != UPDATE_STAGED) return -1;
    upd_state = UPDATE_APPLYING;
    for (i = 0; i < upd_n; i++) {
        upd_tab[i].applied = 0;
    }
    upd_state = UPDATE_OK;
    return 0;
}

int update64_commit(void) {
    int i;
    if (upd_state != UPDATE_STAGED) return -1;
    if (update64_verify() != 0) { upd_state = UPDATE_FAILED; return -2; }
    upd_state = UPDATE_APPLYING;
    for (i = 0; i < upd_n; i++)
        upd_tab[i].applied = 1;
    upd_n = 0;
    upd_state = UPDATE_OK;
    return 0;
}

int update64_state(int *out) {
    if (!out) return -1;
    *out = upd_state;
    return 0;
}

int update64_staged_count(void) {
    return upd_n;
}