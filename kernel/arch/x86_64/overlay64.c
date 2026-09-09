/* 50H: overlayfs — alt katmanlar + ust + beyaz-dosya + birlesik bakis. */
#include "arch/x86_64/longmode.h"

#define OVERLAY64_MAX_LOWER 8
#define OVERLAY64_MAX_WHITE 64
#define OVERLAY64_MAX_UP 64

static char overlay64_lowers[OVERLAY64_MAX_LOWER][64];
static int overlay64_nlower = 0;
static char overlay64_upper[64];
static char overlay64_workdir[64];
static int overlay64_mounted = 0;
static char overlay64_white[OVERLAY64_MAX_WHITE][64];
static int overlay64_nwhite = 0;
static char overlay64_upfiles[OVERLAY64_MAX_UP][64];
static int overlay64_nup = 0;

static void ov_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int ov_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int overlay64_mount(const char *upper, const char *workdir) {
    if (!upper || !workdir) return -1;
    ov_str_copy(overlay64_upper, upper, 64);
    ov_str_copy(overlay64_workdir, workdir, 64);
    overlay64_mounted = 1;
    return 0;
}

int overlay64_add_lower(const char *path) {
    if (!overlay64_mounted || !path) return -1;
    if (overlay64_nlower >= OVERLAY64_MAX_LOWER) return -2;
    ov_str_copy(overlay64_lowers[overlay64_nlower++], path, 64);
    return 0;
}

int overlay64_whiteout(const char *path) {
    int i;
    if (!overlay64_mounted || !path) return -1;
    for (i = 0; i < overlay64_nwhite; i++) {
        if (ov_str_eq(overlay64_white[i], path)) return 0;
    }
    if (overlay64_nwhite >= OVERLAY64_MAX_WHITE) return -2;
    ov_str_copy(overlay64_white[overlay64_nwhite++], path, 64);
    return 0;
}

static int overlay64_hidden(const char *path) {
    int i;
    for (i = 0; i < overlay64_nwhite; i++) {
        if (ov_str_eq(overlay64_white[i], path)) return 1;
    }
    return 0;
}

/* Birlesik bakis: ustte varsa "upper:", yoksa ilk alt katman. */
int overlay64_lookup(const char *path, char *layer_out, int max) {
    int i;
    const char *hit = 0;
    if (!overlay64_mounted || !path) return -1;
    if (overlay64_hidden(path)) return -2;
    for (i = 0; i < overlay64_nup; i++) {
        if (ov_str_eq(overlay64_upfiles[i], path)) {
            hit = overlay64_upper;
            break;
        }
    }
    if (!hit && overlay64_nlower > 0) hit = overlay64_lowers[0];
    if (!hit) return -3;
    if (layer_out && max > 0) ov_str_copy(layer_out, hit, max);
    return 0;
}

int overlay64_copy_up(const char *path) {
    int i;
    if (!overlay64_mounted || !path) return -1;
    for (i = 0; i < overlay64_nup; i++) {
        if (ov_str_eq(overlay64_upfiles[i], path)) return 0;
    }
    if (overlay64_nup >= OVERLAY64_MAX_UP) return -2;
    ov_str_copy(overlay64_upfiles[overlay64_nup++], path, 64);
    return 0;
}
