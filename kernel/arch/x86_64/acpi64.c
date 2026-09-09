/* 34C: ACPI 64-bit — RSDP/XSDT saf ayrıştırma (salt-okunur tampon).
 * Fiziksel adresler testte host pointer olarak verilebilir.
 */
#include "arch/x86_64/longmode.h"

int acpi64_checksum(const void *p, u64 len) {
    const unsigned char *b = (const unsigned char *)p;
    unsigned int sum = 0;
    u64 i;
    if (!b || !len) return -1;
    for (i = 0; i < len; i++) sum += b[i];
    return (sum & 0xFF) == 0 ? 0 : -2;
}

int acpi64_rsdp_check(const void *rsdp, u64 len) {
    const unsigned char *b = (const unsigned char *)rsdp;
    if (!b || len < 20) return -1;
    if (!(b[0] == 'R' && b[1] == 'S' && b[2] == 'D' && b[3] == ' ' &&
          b[4] == 'P' && b[5] == 'T' && b[6] == 'R' && b[7] == ' '))
        return -2;
    if (acpi64_checksum(b, 20) != 0) return -3; /* v1 checksum */
    if (len >= 36 && b[15] >= 2) {
        if (acpi64_checksum(b, 36) != 0) return -4; /* XSDT checksum */
    }
    return 0;
}

/* XSDT: 36B header + u64 girdiler. idx'inci adresi out'a yazar. */
int acpi64_xsdt_entry(const void *xsdt, u64 len, int idx, u64 *out) {
    const unsigned char *b = (const unsigned char *)xsdt;
    u64 hdr_len, count, off;
    int i;
    if (!b || len < 36 || idx < 0) return -1;
    hdr_len = (u64)b[4] | ((u64)b[5] << 8) | ((u64)b[6] << 16) | ((u64)b[7] << 24);
    if (hdr_len > len || hdr_len < 36) return -2;
    count = (hdr_len - 36) / 8;
    if ((u64)idx >= count) return -3;
    off = 36 + (u64)idx * 8;
    if (off + 8 > len) return -4;
    if (out) {
        u64 v = 0;
        for (i = 0; i < 8; i++) v |= (u64)b[off + (u64)i] << (i * 8);
        *out = v;
    }
    return 0;
}

/* sig (4 harf, örn "FACP") tasiyan ilk tabloyu bulur.
 * Sozlesme: XSDT girdileri erisilebilir olmalidir (gercek kernel64'te
 * paging64 ile onceden eslenir; bos (0) girdiler atlanir). */
int acpi64_find(const void *xsdt, u64 len, const char sig[4], u64 *out) {
    const unsigned char *b = (const unsigned char *)xsdt;
    u64 hdr_len, count;
    u64 i;
    if (!b || !sig || len < 36) return -1;
    hdr_len = (u64)b[4] | ((u64)b[5] << 8) | ((u64)b[6] << 16) | ((u64)b[7] << 24);
    if (hdr_len > len || hdr_len < 36) return -2;
    count = (hdr_len - 36) / 8;
    for (i = 0; i < count; i++) {
        u64 addr = 0;
        const unsigned char *t;
        if (acpi64_xsdt_entry(xsdt, len, (int)i, &addr) != 0) continue;
        if (!addr) continue;
        t = (const unsigned char *)(addr);
        if (t[0] == (unsigned char)sig[0] && t[1] == (unsigned char)sig[1] &&
            t[2] == (unsigned char)sig[2] && t[3] == (unsigned char)sig[3]) {
            if (out) *out = addr;
            return 0;
        }
    }
    return -3; /* bulunamadi */
}
