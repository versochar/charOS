/* 35G: KSM skeleton — icerik karmasiyla es sayfa birlestirme.
 * FNV-1a karmasi; es bulunan ikinci sayfa CoW ile paylasilir.
 */
#include "arch/x86_64/longmode.h"

#define KSM64_MAX 128

struct ksm64_entry {
    int used;
    u64 hash;
    u64 frame;
};

static struct ksm64_entry ksm64_tab[KSM64_MAX];
static u64 ksm64_shared_n = 0;
static u64 ksm64_saved_n = 0;
static int ksm64_ready = 0;

static u64 ksm64_hash(const unsigned char *p) {
    u64 h = 1469598103934665603ULL;
    u64 i;
    for (i = 0; i < PMM64_FRAME; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

int ksm64_init(void) {
    int i;
    for (i = 0; i < KSM64_MAX; i++) {
        ksm64_tab[i].used = 0;
        ksm64_tab[i].hash = 0;
        ksm64_tab[i].frame = 0;
    }
    ksm64_shared_n = 0;
    ksm64_saved_n = 0;
    ksm64_ready = 1;
    return 0;
}

/* 1=birlesti (frame paylasildi), 0=yeni kayit, <0 hata */
int ksm64_scan(u64 frame) {
    unsigned char *p;
    u64 h;
    int i, free = -1;
    if (!ksm64_ready || !frame) return -1;
    p = (unsigned char *)pmm64_frame_ptr(frame);
    h = ksm64_hash(p);
    for (i = 0; i < KSM64_MAX; i++) {
        if (!ksm64_tab[i].used) {
            if (free < 0) free = i;
            continue;
        }
        if (ksm64_tab[i].hash == h && ksm64_tab[i].frame != frame) {
            /* Icerik es: paylas (CoW sayaci) + bu frame'i bosa cikar */
            cow64_retain(ksm64_tab[i].frame);
            pmm64_free_frame(frame);
            ksm64_shared_n++;
            ksm64_saved_n++;
            return 1;
        }
    }
    if (free < 0) return -2; /* tablo dolu */
    ksm64_tab[free].used = 1;
    ksm64_tab[free].hash = h;
    ksm64_tab[free].frame = frame;
    return 0;
}

u64 ksm64_shared(void) { return ksm64_shared_n; }
u64 ksm64_saved(void) { return ksm64_saved_n; }
