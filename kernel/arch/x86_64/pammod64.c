/* 38H: PAM modul API — kayit + yerlesik moduller.
 * Faz: 0=auth, 1=acct. Modul imzasi pam64_fn ile ayni.
 */
#include "arch/x86_64/longmode.h"

#define PAMMOD64_MAX 16

struct pammod64_entry {
    int used;
    char name[32];
    pam64_fn auth;
    pam64_fn acct;
};

static struct pammod64_entry pammod64_tab[PAMMOD64_MAX];

static void mod_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

/* Yerlesik: her zaman izin / her zaman red */
static int mod_permit(int handle, const char *user) {
    (void)handle;
    (void)user;
    return 0;
}

static int mod_deny(int handle, const char *user) {
    (void)handle;
    (void)user;
    return -1;
}

/* Yerlesik unix: shadow kaydi VARSA basarili (sifre karsilastirma 38A'da).
 * Skeleton sozlesmesi: kayitli kullanici = auth gecer. */
static int mod_unix_auth(int handle, const char *user) {
    u32 uid, gid;
    (void)handle;
    if (!user) return -1;
    return passwd64_find(user, &uid, &gid) == 0 ? 0 : -1;
}

static int mod_unix_acct(int handle, const char *user) {
    (void)handle;
    (void)user;
    return 0;
}

static void pammod64_builtin(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    pammod64_register("permit", mod_permit, mod_permit);
    pammod64_register("deny", mod_deny, mod_deny);
    pammod64_register("unix", mod_unix_auth, mod_unix_acct);
}

int pammod64_register(const char *name, pam64_fn auth, pam64_fn acct) {
    int i;
    if (!name) return -1;
    for (i = 0; i < PAMMOD64_MAX; i++) {
        if (pammod64_tab[i].used) {
            int j, same = 1;
            for (j = 0; name[j] || pammod64_tab[i].name[j]; j++) {
                if (name[j] != pammod64_tab[i].name[j]) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                pammod64_tab[i].auth = auth;
                pammod64_tab[i].acct = acct;
                return 0;
            }
        }
    }
    for (i = 0; i < PAMMOD64_MAX; i++) {
        if (!pammod64_tab[i].used) {
            pammod64_tab[i].used = 1;
            mod_str_copy(pammod64_tab[i].name, name, 32);
            pammod64_tab[i].auth = auth;
            pammod64_tab[i].acct = acct;
            return 0;
        }
    }
    return -2;
}

pam64_fn pammod64_find(const char *name, int phase) {
    int i, j;
    pammod64_builtin();
    if (!name) return 0;
    for (i = 0; i < PAMMOD64_MAX; i++) {
        int same;
        if (!pammod64_tab[i].used) continue;
        same = 1;
        for (j = 0; name[j] || pammod64_tab[i].name[j]; j++) {
            if (name[j] != pammod64_tab[i].name[j]) {
                same = 0;
                break;
            }
        }
        if (!same) continue;
        return phase ? pammod64_tab[i].acct : pammod64_tab[i].auth;
    }
    return 0;
}
