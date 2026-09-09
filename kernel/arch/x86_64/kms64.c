/* 47B: KMS — baglayici/kip tablosu + gecerli kip. */
#include "arch/x86_64/longmode.h"

#define KMS64_MAX_CONN 8
#define KMS64_MAX_MODES 16

struct kms64_mode {
    u32 w, h, refresh;
};

struct kms64_conn {
    int used;
    char name[16];
    struct kms64_mode modes[KMS64_MAX_MODES];
    int nmodes;
    int cur; /* -1 = kapali */
};

static struct kms64_conn kms64_tab[KMS64_MAX_CONN];

static void kms_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int kms64_add_connector(const char *name) {
    int i;
    if (!name) return -1;
    for (i = 0; i < KMS64_MAX_CONN; i++) {
        if (!kms64_tab[i].used) {
            kms64_tab[i].used = 1;
            kms_str_copy(kms64_tab[i].name, name, 16);
            kms64_tab[i].nmodes = 0;
            kms64_tab[i].cur = -1;
            return i;
        }
    }
    return -2;
}

int kms64_add_mode(int conn, u32 w, u32 h, u32 refresh) {
    struct kms64_conn *c;
    if (conn < 0 || conn >= KMS64_MAX_CONN || !kms64_tab[conn].used)
        return -1;
    if (!w || !h || !refresh || w > 4096 || h > 4096) return -1;
    c = &kms64_tab[conn];
    if (c->nmodes >= KMS64_MAX_MODES) return -2;
    c->modes[c->nmodes].w = w;
    c->modes[c->nmodes].h = h;
    c->modes[c->nmodes].refresh = refresh;
    c->nmodes++;
    return 0;
}

int kms64_set_mode(int conn, u32 w, u32 h) {
    struct kms64_conn *c;
    int i;
    if (conn < 0 || conn >= KMS64_MAX_CONN || !kms64_tab[conn].used)
        return -1;
    c = &kms64_tab[conn];
    for (i = 0; i < c->nmodes; i++) {
        if (c->modes[i].w == w && c->modes[i].h == h) {
            c->cur = i;
            return 0;
        }
    }
    return -2; /* listede yok */
}

int kms64_current(int conn, u32 *w, u32 *h, u32 *refresh) {
    struct kms64_conn *c;
    if (conn < 0 || conn >= KMS64_MAX_CONN || !kms64_tab[conn].used)
        return -1;
    c = &kms64_tab[conn];
    if (c->cur < 0) return -2;
    if (w) *w = c->modes[c->cur].w;
    if (h) *h = c->modes[c->cur].h;
    if (refresh) *refresh = c->modes[c->cur].refresh;
    return 0;
}
