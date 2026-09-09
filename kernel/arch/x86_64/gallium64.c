/* 47A: Gallium low profile — yetenek kaps + bicim tablosu (512MB hedef). */
#include "arch/x86_64/longmode.h"

#define GALLIUM64_FMT_RGBA8 1
#define GALLIUM64_FMT_Z24 2
#define GALLIUM64_FMT_DXT1 3
#define GALLIUM64_FMT_DXT5 4
#define GALLIUM64_FMT_R16F 5

static u64 gallium64_vram = 0;
static int gallium64_ready = 0;

int gallium64_init(u64 vram_mb) {
    if (!vram_mb || vram_mb > 8192) return -1;
    gallium64_vram = vram_mb;
    gallium64_ready = 1;
    return 0;
}

int gallium64_caps(u32 *max_tex, int *low_profile) {
    if (!gallium64_ready) return -1;
    /* Dusuk profil: 2K doku, blit oncelikli */
    if (max_tex) *max_tex = 2048;
    if (low_profile) *low_profile = (gallium64_vram <= 1024) ? 1 : 0;
    return 0;
}

int gallium64_format_ok(u32 format) {
    if (!gallium64_ready) return 0;
    if (format == GALLIUM64_FMT_RGBA8 || format == GALLIUM64_FMT_Z24)
        return 1;
    /* Sikistirilmis: yalniz yeterli VRAM'de */
    if ((format == GALLIUM64_FMT_DXT1 || format == GALLIUM64_FMT_DXT5) &&
        gallium64_vram >= 256)
        return 1;
    if (format == GALLIUM64_FMT_R16F && gallium64_vram >= 512) return 1;
    return 0;
}
