/* 53C: NSS — veritabani/modul + anahtar-deger cozumu. */
#include "arch/x86_64/longmode.h"

#define NSS64_MAX_SVC 16
#define NSS64_MAX_ENTRIES 64

struct nss64_svc {
    int used;
    char db[32];
    char modules[4][32];
    int nmodules;
};

struct nss64_entry {
    int used;
    char db[64];
    char module[32];
    char key[64];
    char value[128];
};

static struct nss64_svc nss64_svcs[NSS64_MAX_SVC];
static struct nss64_entry nss64_tab[NSS64_MAX_ENTRIES];

static void nss_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int nss_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int nss64_add(const char *db, const char *module) {
    int i;
    if (!db || !module) return -1;
    for (i = 0; i < NSS64_MAX_SVC; i++) {
        if (nss64_svcs[i].used && nss_str_eq(nss64_svcs[i].db, db)) {
            int m;
            for (m = 0; m < nss64_svcs[i].nmodules; m++) {
                if (nss_str_eq(nss64_svcs[i].modules[m], module))
                    return 0;
            }
            if (nss64_svcs[i].nmodules >= 4) return -2;
            nss_str_copy(
                nss64_svcs[i].modules[nss64_svcs[i].nmodules++], module,
                32);
            return 0;
        }
    }
    for (i = 0; i < NSS64_MAX_SVC; i++) {
        if (!nss64_svcs[i].used) {
            nss64_svcs[i].used = 1;
            nss_str_copy(nss64_svcs[i].db, db, 32);
            nss_str_copy(nss64_svcs[i].modules[0], module, 32);
            nss64_svcs[i].nmodules = 1;
            return 0;
        }
    }
    return -3;
}

int nss64_module_add(const char *db, const char *module, const char *key,
                     const char *value) {
    int i;
    if (!db || !module || !key) return -1;
    for (i = 0; i < NSS64_MAX_ENTRIES; i++) {
        if (!nss64_tab[i].used) {
            nss64_tab[i].used = 1;
            nss_str_copy(nss64_tab[i].db, db, 64);
            nss_str_copy(nss64_tab[i].module, module, 32);
            nss_str_copy(nss64_tab[i].key, key, 64);
            nss_str_copy(nss64_tab[i].value, value ? value : "", 128);
            return 0;
        }
    }
    return -2;
}

/* Servis modul sirasinda ilk eslesme kazanir. */
int nss64_lookup(const char *db, const char *key, char *out, int max) {
    int s, m, i;
    if (!db || !key) return -1;
    for (s = 0; s < NSS64_MAX_SVC; s++) {
        if (!nss64_svcs[s].used || !nss_str_eq(nss64_svcs[s].db, db))
            continue;
        for (m = 0; m < nss64_svcs[s].nmodules; m++) {
            for (i = 0; i < NSS64_MAX_ENTRIES; i++) {
                if (!nss64_tab[i].used) continue;
                if (!nss_str_eq(nss64_tab[i].db, db)) continue;
                if (!nss_str_eq(nss64_tab[i].module,
                                nss64_svcs[s].modules[m]))
                    continue;
                if (!nss_str_eq(nss64_tab[i].key, key)) continue;
                if (out && max > 0)
                    nss_str_copy(out, nss64_tab[i].value, max);
                return 0;
            }
        }
        return -2; /* servis var, kayit yok */
    }
    return -3; /* servis yok */
}
