/* 47I: VA-API decode — profil + yapilandirma + yuzey havuzu. */
#include "arch/x86_64/longmode.h"

#define VAAPI64_H264 0
#define VAAPI64_HEVC 1
#define VAAPI64_VP9 2
#define VAAPI64_MAX_SURF 32

struct vaapi64_surf {
    int used;
    u32 w, h;
};

static struct vaapi64_surf vaapi64_pool[VAAPI64_MAX_SURF];
static int vaapi64_profile = -1;
static u32 vaapi64_w = 0, vaapi64_h = 0;
static u64 vaapi64_frames_n = 0;
static u64 vaapi64_checksum = 0;

int vaapi64_profile_ok(int profile) {
    return (profile == VAAPI64_H264 || profile == VAAPI64_HEVC ||
            profile == VAAPI64_VP9)
               ? 1
               : 0;
}

int vaapi64_config(int profile, u32 w, u32 h) {
    if (!vaapi64_profile_ok(profile)) return -1;
    if (!w || !h || w > 4096 || h > 4096) return -1;
    vaapi64_profile = profile;
    vaapi64_w = w;
    vaapi64_h = h;
    return 0;
}

int vaapi64_surfaces(int n, u32 w, u32 h) {
    int i, made = 0;
    if (n <= 0 || !w || !h) return -1;
    for (i = 0; i < VAAPI64_MAX_SURF && made < n; i++) {
        if (!vaapi64_pool[i].used) {
            vaapi64_pool[i].used = 1;
            vaapi64_pool[i].w = w;
            vaapi64_pool[i].h = h;
            made++;
        }
    }
    return made == n ? 0 : -2;
}

int vaapi64_decode(int surf, const void *data, u64 len) {
    const unsigned char *p;
    u64 i;
    if (surf < 0 || surf >= VAAPI64_MAX_SURF || !vaapi64_pool[surf].used)
        return -1;
    if (!data || !len) return -2;
    if (vaapi64_profile < 0) return -3;
    p = (const unsigned char *)data;
    for (i = 0; i < len; i++) {
        vaapi64_checksum += p[i];
        vaapi64_checksum *= 1099511628211ULL;
    }
    vaapi64_frames_n++;
    return 0;
}

u64 vaapi64_frames(void) { return vaapi64_frames_n; }
