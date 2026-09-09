/* 38B: PAM iskeleti — servis yigini + kontrol bayraklari.
 * required: basarisizsa zincir biter (hata). requisite: hemen doner.
 * sufficient: basariliysa (onceki required hatasizsa) hemen basari.
 * optional: sonucu yoksay.
 */
#include "arch/x86_64/longmode.h"

#define PAM64_MAX_HANDLES 8
#define PAM64_MAX_MODS 8

struct pam64_mod {
    pam64_fn fn;
    int control;
};

struct pam64_handle {
    int used;
    char service[32];
    char user[32];
};

static struct pam64_handle pam64_tab[PAM64_MAX_HANDLES];
static struct pam64_mod pam64_chain[PAM64_MAX_HANDLES][PAM64_MAX_MODS];
static int pam64_nmods[PAM64_MAX_HANDLES];

static void pam_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int pam64_start(const char *service, const char *user) {
    int i;
    if (!service || !user) return -1;
    for (i = 0; i < PAM64_MAX_HANDLES; i++) {
        if (!pam64_tab[i].used) {
            pam64_tab[i].used = 1;
            pam_str_copy(pam64_tab[i].service, service, 32);
            pam_str_copy(pam64_tab[i].user, user, 32);
            pam64_nmods[i] = 0;
            return i;
        }
    }
    return -2;
}

int pam64_add(int handle, pam64_fn fn, int control) {
    int n;
    if (handle < 0 || handle >= PAM64_MAX_HANDLES) return -1;
    if (!pam64_tab[handle].used || !fn) return -1;
    if (control < 0 || control > 3) return -1;
    n = pam64_nmods[handle];
    if (n >= PAM64_MAX_MODS) return -2;
    pam64_chain[handle][n].fn = fn;
    pam64_chain[handle][n].control = control;
    pam64_nmods[handle]++;
    return 0;
}

/* 0=basari, <0 hata */
static int pam64_run(int handle, int phase) {
    int i, failed = 0;
    (void)phase; /* auth/acct ayrimi modulde (38H) */
    if (handle < 0 || handle >= PAM64_MAX_HANDLES) return -1;
    if (!pam64_tab[handle].used) return -1;
    for (i = 0; i < pam64_nmods[handle]; i++) {
        int ctl = pam64_chain[handle][i].control;
        int rc = pam64_chain[handle][i].fn(handle,
                                           pam64_tab[handle].user);
        if (rc == 0) {
            if (ctl == PAM64_SUFFICIENT && !failed) return 0;
            continue;
        }
        if (ctl == PAM64_OPTIONAL || ctl == PAM64_SUFFICIENT) continue;
        if (ctl == PAM64_REQUISITE) return -2;
        failed = 1; /* REQUIRED: devam et ama hata kalir */
    }
    return failed ? -3 : 0;
}

int pam64_authenticate(int handle) { return pam64_run(handle, 0); }
int pam64_acct_mgmt(int handle) { return pam64_run(handle, 1); }

void pam64_end(int handle) {
    if (handle < 0 || handle >= PAM64_MAX_HANDLES) return;
    pam64_tab[handle].used = 0;
    pam64_nmods[handle] = 0;
}
