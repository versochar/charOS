/* 48A: ACPI parse — XSDT'ten FACP bul + alan cikarimi (acpi64 ustu). */
#include "arch/x86_64/longmode.h"

static u32 ap_rd32(const unsigned char *p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

int acpiparse64_fadt(const void *xsdt, u64 len,
                     struct acpiparse64_fadt *out) {
    u64 addr = 0;
    const unsigned char *f;
    if (!xsdt || !out) return -1;
    if (acpi64_find(xsdt, len, "FACP", &addr) != 0) return -2;
    f = (const unsigned char *)addr;
    out->dsdt = ap_rd32(f + 40);
    out->smi_cmd = ap_rd32(f + 48);
    out->pm1a_cnt = ap_rd32(f + 64);
    out->pm1b_cnt = ap_rd32(f + 68);
    /* SLP_TYP gercekte _S5 AML'den gelir; skeleton varsayilani (48B). */
    out->slp_typ_a = 5;
    out->slp_typ_b = 5;
    if (!out->pm1a_cnt) return -3; /* PM1 yoksa uyku yok */
    return 0;
}

int acpiparse64_sleep(const struct acpiparse64_fadt *fadt, u8 *a, u8 *b) {
    if (!fadt) return -1;
    if (!fadt->pm1a_cnt) return -2;
    if (a) *a = fadt->slp_typ_a;
    if (b) *b = fadt->slp_typ_b;
    return 0;
}
