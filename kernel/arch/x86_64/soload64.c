/* 52D: .so yukleme plani — ET_DYN bias + kesit eslemesi. */
#include "arch/x86_64/longmode.h"
#include "elf64.h"

int soload64_plan(const void *buf, u64 len, u64 bias,
                  struct soload64_seg *segs, int max, int *count_out,
                  u64 *total_out) {
    const unsigned char *b = (const unsigned char *)buf;
    const Elf64_Ehdr *eh;
    const Elf64_Phdr *ph;
    Elf64_Addr entry;
    u64 lo = ~0ULL, hi = 0;
    int i, n = 0;
    if (elf64_check(buf, len, &entry) != 0) return -1;
    if (!segs || max <= 0) return -1;
    if (bias & 0xFFFULL) return -2; /* sayfa hizali sart */
    eh = (const Elf64_Ehdr *)buf;
    ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz) continue;
        if (n >= max) return -3;
        segs[n].vaddr = ph[i].p_vaddr + bias;
        segs[n].memsz = ph[i].p_memsz;
        segs[n].filesz = ph[i].p_filesz;
        segs[n].flags = ph[i].p_flags;
        if (segs[n].vaddr < lo) lo = segs[n].vaddr;
        if (segs[n].vaddr + segs[n].memsz > hi)
            hi = segs[n].vaddr + segs[n].memsz;
        n++;
    }
    if (!n) return -4;
    if (eh->e_type == 2) { /* ET_EXEC: bias sifir olmali */
        if (bias) return -5;
    }
    if (count_out) *count_out = n;
    if (total_out) *total_out = hi - lo;
    return 0;
}
