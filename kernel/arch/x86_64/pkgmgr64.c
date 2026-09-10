#include "arch/x86_64/pkgmgr.h"
#include <string.h>

struct pkg_entry {
    char name[PKGMGR64_NAME_MAX];
    char version[PKGMGR64_NAME_MAX];
    u8   installed;
};
static struct pkg_entry pkg_tab[PKGMGR64_MAX_PKGS];
static int pkg_n = 0;
static int pkg_state = PKGMGR_IDLE;

static int pkg_ver_cmp(const char *a, const char *b) {
    const char *pa = a, *pb = b;
    int ha = 0, hb = 0;
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
    if (*pa) ha = 1;
    if (*pb) hb = 1;
    return ha - hb;
}

int pkgmgr64_init(void) {
    pkg_n = 0;
    pkg_state = PKGMGR_IDLE;
    return 0;
}

int pkgmgr64_install(const char *name, const char *version) {
    int i;
    if (!name || !version) return -1;
    if (pkg_state == PKGMGR_OP_IN_PROGRESS) return -3;
    for (i = 0; i < pkg_n; i++) {
        if (pkg_tab[i].installed && strcmp(pkg_tab[i].name, name) == 0) {
            if (pkg_ver_cmp(version, pkg_tab[i].version) <= 0)
                return -2; /* zaten yuklenmis / daha eski surum */
            strncpy(pkg_tab[i].version, version, sizeof(pkg_tab[i].version) - 1);
            return 0; /* upgrade */
        }
    }
    if (pkg_n >= PKGMGR64_MAX_PKGS) { pkg_state = PKGMGR_FULL; return -4; }
    memset(&pkg_tab[pkg_n], 0, sizeof(pkg_tab[pkg_n]));
    strncpy(pkg_tab[pkg_n].name, name, sizeof(pkg_tab[pkg_n].name) - 1);
    strncpy(pkg_tab[pkg_n].version, version, sizeof(pkg_tab[pkg_n].version) - 1);
    pkg_tab[pkg_n].installed = 1;
    pkg_n++;
    return 0;
}

int pkgmgr64_remove(const char *name) {
    int i;
    if (!name) return -1;
    if (pkg_state == PKGMGR_OP_IN_PROGRESS) return -3;
    for (i = 0; i < pkg_n; i++) {
        if (pkg_tab[i].installed && strcmp(pkg_tab[i].name, name) == 0) {
            pkg_tab[i].installed = 0;
            return 0;
        }
    }
    return -2; /* kurulu degil */
}

int pkgmgr64_query(const char *name, char *version, int max) {
    int i;
    if (!name || !version || max <= 0) return -1;
    for (i = 0; i < pkg_n; i++) {
        if (pkg_tab[i].installed && strcmp(pkg_tab[i].name, name) == 0) {
            strncpy(version, pkg_tab[i].version, (size_t)max - 1);
            version[max - 1] = '\0';
            return 0;
        }
    }
    return -2;
}

int pkgmgr64_upgrade_all(void) {
    int i;
    if (pkg_state == PKGMGR_OP_IN_PROGRESS) return -3;
    pkg_state = PKGMGR_OP_IN_PROGRESS;
    /* davranis modeli: boyut dogrulama gecildigini varsayar */
    pkg_state = PKGMGR_IDLE;
    for (i = 0; i < pkg_n; i++)
        (void)i;
    return pkg_n;
}

int pkgmgr64_list_names(char *out[], int max) {
    int i, c = 0;
    if (!out || max <= 0) return -1;
    for (i = 0; i < pkg_n && c < max; i++) {
        if (pkg_tab[i].installed) {
            out[c++] = pkg_tab[i].name;
        }
    }
    return c;
}

int pkgmgr64_installed_count(void) {
    int i, c = 0;
    for (i = 0; i < pkg_n; i++)
        if (pkg_tab[i].installed) c++;
    return c;
}

int pkgmgr64_state(int *out) {
    if (!out) return -1;
    *out = pkg_state;
    return 0;
}