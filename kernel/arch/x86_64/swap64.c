/* 32H/32I: swap — backend arayuzu + slot yonetimi + RAM-disk backend.
 * 8192 slot bitmap. Saf C, asm yok (host testi uygun).
 */
#include "arch/x86_64/longmode.h"

#define SWAP64_MAX_SLOTS 8192
#define SWAP64_BITMAP_WORDS (SWAP64_MAX_SLOTS / 64)

static const struct swap64_ops *swap64_ops = 0;
static u64 swap64_bitmap[SWAP64_BITMAP_WORDS];
static u64 swap64_nslots = 0;

/* 32I: RAM-disk backend (swap partition taramasi 33+/40x'te gercek diske) */
static u64 swap64_ramdisk_frames[SWAP64_MAX_SLOTS];
static u64 swap64_ramdisk_n = 0;

static int ramdisk_read(u64 slot, void *buf) {
    unsigned char *d, *s;
    u64 i;
    if (slot >= swap64_ramdisk_n || !buf) return -1;
    d = (unsigned char *)buf;
    s = (unsigned char *)pmm64_frame_ptr(swap64_ramdisk_frames[slot]);
    for (i = 0; i < PMM64_FRAME; i++) d[i] = s[i];
    return 0;
}

static int ramdisk_write(u64 slot, const void *buf) {
    unsigned char *d;
    const unsigned char *s;
    u64 i;
    if (slot >= swap64_ramdisk_n || !buf) return -1;
    d = (unsigned char *)pmm64_frame_ptr(swap64_ramdisk_frames[slot]);
    s = (const unsigned char *)buf;
    for (i = 0; i < PMM64_FRAME; i++) d[i] = s[i];
    return 0;
}

int swap64_register(const struct swap64_ops *ops) {
    u64 i;
    if (!ops || !ops->read_slot || !ops->write_slot || !ops->slots) return -1;
    if (ops->slots > SWAP64_MAX_SLOTS) return -2;
    for (i = 0; i < SWAP64_BITMAP_WORDS; i++) swap64_bitmap[i] = 0;
    swap64_ops = ops;
    swap64_nslots = ops->slots;
    return 0;
}

int swap64_alloc_slot(u64 *slot_out) {
    u64 i;
    if (!swap64_ops) return -1;
    for (i = 0; i < SWAP64_MAX_SLOTS; i++) {
        if (i >= swap64_nslots) break;
        if (!(swap64_bitmap[i >> 6] & (1ULL << (i & 63)))) {
            swap64_bitmap[i >> 6] |= (1ULL << (i & 63));
            if (slot_out) *slot_out = i;
            return 0;
        }
    }
    return -2; /* dolu */
}

void swap64_free_slot(u64 slot) {
    if (!swap64_ops || slot >= swap64_nslots) return;
    swap64_bitmap[slot >> 6] &= ~(1ULL << (slot & 63));
}

int swap64_in(u64 slot, void *frame_buf) {
    if (!swap64_ops || slot >= swap64_nslots || !frame_buf) return -1;
    return swap64_ops->read_slot(slot, frame_buf);
}

int swap64_out(u64 slot, const void *frame_buf) {
    if (!swap64_ops || slot >= swap64_nslots || !frame_buf) return -1;
    return swap64_ops->write_slot(slot, frame_buf);
}

int swap64_swapon_ramdisk(u64 slots) {
    static struct swap64_ops ops;
    u64 i;
    if (!slots || slots > SWAP64_MAX_SLOTS) return -1;
    for (i = 0; i < slots; i++) {
        swap64_ramdisk_frames[i] = pmm64_alloc_frame();
        if (!swap64_ramdisk_frames[i]) {
            /* Kismi tahsisi geri ver */
            while (i-- > 0) pmm64_free_frame(swap64_ramdisk_frames[i]);
            return -2;
        }
    }
    swap64_ramdisk_n = slots;
    ops.read_slot = ramdisk_read;
    ops.write_slot = ramdisk_write;
    ops.slots = slots;
    return swap64_register(&ops);
}
