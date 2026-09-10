#include "arch/x86_64/isoimg.h"
#include <string.h>

struct iso_file {
    char name[ISOIMG64_NAME_MAX];
    u64  size;
    u32  sector;      /* baslangic sektoru */
    u32  nsectors;
    u8   is_boot;
};
static struct iso_file iso_tab[ISOIMG64_MAX_FILES];
static int iso_n = 0;
static int iso_boot = ISOIMG_NONE;
static u64 iso_layout_size = 0;
static u32 iso_magic_state = 0;
static char iso_boot_path[ISOIMG64_NAME_MAX];

static u32 iso_hash(const char *s) {
    u32 h = 2166136261u;
    const unsigned char *p = (const unsigned char *)s;
    while (*p) { h ^= *p++; h *= 16777619u; }
    return h;
}

int isoimg64_init(void) {
    iso_n = 0;
    iso_boot = ISOIMG_NONE;
    iso_layout_size = 0;
    iso_magic_state = ISOIMG64_MAGIC;
    memset(iso_boot_path, 0, sizeof(iso_boot_path));
    memset(iso_tab, 0, sizeof(iso_tab));
    return 0;
}

int isoimg64_add_file(const char *name, u64 size_bytes) {
    int i;
    if (!name) return -1;
    if (iso_n >= ISOIMG64_MAX_FILES) return -2;
    if (size_bytes == 0 || size_bytes > 0x100000000ULL) return -3;
    for (i = 0; i < iso_n; i++) {
        if (strcmp(iso_tab[i].name, name) == 0)
            return -4; /* ayni isim iki kez */
    }
    memset(&iso_tab[iso_n], 0, sizeof(iso_tab[iso_n]));
    strncpy(iso_tab[iso_n].name, name, sizeof(iso_tab[iso_n].name) - 1);
    iso_tab[iso_n].size = size_bytes;
    iso_tab[iso_n].nsectors =
        (u32)((size_bytes + ISOIMG64_SECT - 1) / ISOIMG64_SECT);
    iso_n++;
    return 0;
}

int isoimg64_set_boot(const char *kernel_path, int boot_mode) {
    int i;
    if (!kernel_path || !boot_mode) return -1;
    if (boot_mode != ISOIMG_ELTORITO_FLOPPY &&
        boot_mode != ISOIMG_ELTORITO_NOT_EMULATE)
        return -2;
    for (i = 0; i < iso_n; i++) {
        if (strcmp(iso_tab[i].name, kernel_path) == 0) {
            iso_tab[i].is_boot = 1;
            strncpy(iso_boot_path, kernel_path,
                    sizeof(iso_boot_path) - 1);
            iso_boot = boot_mode;
            return 0;
        }
    }
    return -3; /* kernel path indexte yok */
}

int isoimg64_layout(u64 *total_bytes) {
    u32 next = 16; /* ilk 16 sektor: system area + PVD */
    u64 total;
    int i;
    if (iso_n == 0) return -1;
    if (iso_boot == ISOIMG_NONE) return -2; /* boot dosyasi sart */
    for (i = 0; i < iso_n; i++) {
        iso_tab[i].sector = next;
        next += iso_tab[i].nsectors;
    }
    total = (u64)next * ISOIMG64_SECT;
    iso_layout_size = total;
    if (total_bytes) *total_bytes = total;
    return 0;
}

int isoimg64_sector_for(const char *name, u32 *sector) {
    int i;
    if (!name || !sector) return -1;
    for (i = 0; i < iso_n; i++) {
        if (strcmp(iso_tab[i].name, name) == 0) {
            if (iso_tab[i].sector == 0 && iso_layout_size == 0)
                return -2; /* henuz layout cagrilmadi */
            *sector = iso_tab[i].sector;
            return 0;
        }
    }
    return -3;
}

int isoimg64_checksum(void) {
    u32 h = iso_magic_state;
    int i;
    u64 k;
    for (i = 0; i < iso_n; i++) {
        h ^= iso_hash(iso_tab[i].name);
        h *= 16777619u;
        k = iso_tab[i].size;
        h ^= (u32)(k & 0xFFFFFFFFu);
        h ^= (u32)(k >> 32);
        h *= 16777619u;
        h ^= iso_tab[i].sector;
        h *= 16777619u;
    }
    h ^= (u32)iso_boot;
    iso_magic_state = h ? h : 1;
    return (int)iso_magic_state;
}

int isoimg64_verify(const char *name) {
    int i;
    u32 sector;
    if (!name) return -1;
    for (i = 0; i < iso_n; i++) {
        if (strcmp(iso_tab[i].name, name) == 0) {
            if (iso_layout_size == 0) return -2; /* layout yok */
            sector = iso_tab[i].sector;
            if (iso_tab[i].nsectors == 0) return -3;
            if (iso_tab[i].is_boot && iso_boot == ISOIMG_NONE)
                return -4;
            (void)sector;
            return 0;
        }
    }
    return -5;
}

int isoimg64_file_count(void) {
    return iso_n;
}

int isoimg64_boot_mode(int *out) {
    if (!out) return -1;
    *out = iso_boot;
    return 0;
}