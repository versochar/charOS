#include "arch/x86_64/fpsb.h"
#include <string.h>

#define FPSB_ALL  0x007Fu

struct fpsb_slot {
    char app_id[FPSB64_APP_ID_MAX];
    char runtime_ref[FPSB64_NAME_MAX];
    u32  perms;
    int  denied;
    int  state;
    int  used;
    int  id;
};
static struct fpsb_slot fpsb_tab[FPSB64_MAX_SANDBOXES];
static int fpsb_next_id = 1;

static int fpsb_find(int id) {
    int i;
    for (i = 0; i < FPSB64_MAX_SANDBOXES; i++)
        if (fpsb_tab[i].used && fpsb_tab[i].id == id)
            return i;
    return -1;
}

int fpsb64_init(void) {
    memset(fpsb_tab, 0, sizeof(fpsb_tab));
    fpsb_next_id = 1;
    return 0;
}

int fpsb64_create(const char *app_id, u32 perms, const char *runtime_ref,
                  int *out_id) {
    int i;
    if (!app_id || !runtime_ref || !out_id) return -1;
    if (perms & ~FPSB_ALL) return -2; /* bilinmeyen izin biti */
    for (i = 0; i < FPSB64_MAX_SANDBOXES; i++) {
        if (fpsb_tab[i].used && strcmp(fpsb_tab[i].app_id, app_id) == 0)
            return -3; /* ayni app zaten kum havuzlu */
    }
    for (i = 0; i < FPSB64_MAX_SANDBOXES; i++) {
        if (!fpsb_tab[i].used) {
            fpsb_tab[i].used = 1;
            fpsb_tab[i].id = fpsb_next_id++;
            strncpy(fpsb_tab[i].app_id, app_id,
                    sizeof(fpsb_tab[i].app_id) - 1);
            strncpy(fpsb_tab[i].runtime_ref, runtime_ref,
                    sizeof(fpsb_tab[i].runtime_ref) - 1);
            fpsb_tab[i].perms = perms;
            fpsb_tab[i].denied = 0;
            fpsb_tab[i].state = FPSB_CREATED;
            *out_id = fpsb_tab[i].id;
            return 0;
        }
    }
    return -4;
}

int fpsb64_launch(int id) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    if (fpsb_tab[i].state != FPSB_CREATED) return -2;
    /* ag + dosya sistemi ayni anda istenirse engel (model kurali) */
    if ((fpsb_tab[i].perms & FPSB_NET) && (fpsb_tab[i].perms & FPSB_DEVICES)) {
        fpsb_tab[i].state = FPSB_BLOCKED;
        fpsb_tab[i].denied++;
        return -3; /* guvensiz kombinasyon */
    }
    fpsb_tab[i].state = FPSB_RUNNING;
    return 0;
}

int fpsb64_grant(int id, u32 bits) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    if (bits & ~FPSB_ALL) return -2;
    if (fpsb_tab[i].state == FPSB_RUNNING) return -3; /* calisirken izin yok */
    fpsb_tab[i].perms |= bits;
    return 0;
}

int fpsb64_revoke(int id, u32 bits) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    if (bits & ~FPSB_ALL) return -2;
    if (fpsb_tab[i].state == FPSB_RUNNING) return -3;
    fpsb_tab[i].perms &= ~bits;
    return 0;
}

int fpsb64_has_perm(int id, u32 bits) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    return (fpsb_tab[i].perms & bits) == bits ? 1 : 0;
}

int fpsb64_state(int id, int *out) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    if (!out) return -1;
    *out = fpsb_tab[i].state;
    return 0;
}

int fpsb64_denied_count(int id, int *out) {
    int i = fpsb_find(id);
    if (i < 0) return -1;
    if (!out) return -1;
    *out = fpsb_tab[i].denied;
    return 0;
}

int fpsb64_count(void) {
    int i, c = 0;
    for (i = 0; i < FPSB64_MAX_SANDBOXES; i++)
        if (fpsb_tab[i].used) c++;
    return c;
}