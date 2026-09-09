/* 45E: xHCI MSI — yetenek cozumu + vektor havuzu + ileti kurma. */
#include "arch/x86_64/longmode.h"

#define MSI64_MAX_VEC 32

static u64 msi64_vecmap = 0;

int msi64_parse(u16 control, int *is64, int *mmc) {
    int c64 = (control & 0x80) ? 1 : 0;
    int cap = (control >> 1) & 0x7;   /* MMC: istenen coklu ileti */
    int en = (control >> 4) & 0x7;    /* MME: etkin */
    if (is64) *is64 = c64;
    if (mmc) *mmc = cap > en ? cap : en;
    return 0;
}

int msi64_alloc_vector(int *vec_out) {
    int i;
    for (i = 0; i < MSI64_MAX_VEC; i++) {
        if (!(msi64_vecmap & (1ULL << i))) {
            msi64_vecmap |= (1ULL << i);
            if (vec_out) *vec_out = 0x40 + i; /* 0x40..0x5F araligi */
            return 0;
        }
    }
    return -1;
}

void msi64_free_vector(int vec) {
    int i = vec - 0x40;
    if (i < 0 || i >= MSI64_MAX_VEC) return;
    msi64_vecmap &= ~(1ULL << i);
}

/* x86 MSI iletisi: adres = 0xFEE00000 | (apic_id << 12), veri = vektor. */
int msi64_msg(u64 *addr, u32 *data, int vec, int apic_id) {
    if (vec < 0x20 || vec > 0xFF || apic_id < 0 || apic_id > 255)
        return -1;
    if (addr) *addr = 0xFEE00000ULL | ((u64)apic_id << 12);
    if (data) *data = (u32)vec;
    return 0;
}

/* MSI-X tablo girdisi (16B): addr-lo, addr-hi, data, ctrl. */
int msi64_msix_entry(const unsigned char *e, u64 *addr, u32 *data,
                     u32 *ctrl) {
    u64 lo, hi;
    int i;
    if (!e) return -1;
    lo = hi = 0;
    for (i = 0; i < 4; i++) {
        lo |= (u64)e[i] << (i * 8);
        hi |= (u64)e[4 + i] << (i * 8);
    }
    if (addr) *addr = lo | (hi << 32);
    if (data) {
        u32 d = 0;
        for (i = 0; i < 4; i++) d |= (u32)e[8 + i] << (i * 8);
        *data = d;
    }
    if (ctrl) {
        u32 c = 0;
        for (i = 0; i < 4; i++) c |= (u32)e[12 + i] << (i * 8);
        *ctrl = c;
    }
    return 0;
}
