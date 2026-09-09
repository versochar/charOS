/* 52B: program headers — yuruyus + INTERP + GNU_STACK. */
#include "arch/x86_64/longmode.h"
#include "elf64.h"

#define PT_INTERP64 3
#define PT_NOTE64 4
#define PT_DYNAMIC64 2
#define PT_GNU_STACK64 0x6474E551

int progheader64_each(const void *buf, u64 len,
                      int (*cb)(u32 type, u64 off, u64 vaddr, u64 filesz,
                                u64 memsz, u32 flags)) {
    const unsigned char *b = (const unsigned char *)buf;
    const Elf64_Ehdr *eh;
    const Elf64_Phdr *ph;
    Elf64_Addr entry;
    int i, n = 0;
    if (elf64_check(buf, len, &entry) != 0) return -1;
    if (!cb) return -2;
    eh = (const Elf64_Ehdr *)buf;
    ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (i = 0; i < eh->e_phnum; i++) {
        int rc = cb(ph[i].p_type, ph[i].p_offset, ph[i].p_vaddr,
                    ph[i].p_filesz, ph[i].p_memsz, ph[i].p_flags);
        if (rc < 0) return -3;
        n++;
    }
    return n;
}

int progheader64_interp(const void *buf, u64 len, char *out, int max) {
    const unsigned char *b = (const unsigned char *)buf;
    const Elf64_Ehdr *eh;
    const Elf64_Phdr *ph;
    Elf64_Addr entry;
    int i;
    if (elf64_check(buf, len, &entry) != 0) return -1;
    if (!out || max <= 0) return -1;
    eh = (const Elf64_Ehdr *)buf;
    ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (i = 0; i < eh->e_phnum; i++) {
        u64 j;
        if (ph[i].p_type != PT_INTERP64) continue;
        if (ph[i].p_offset + ph[i].p_filesz > len) return -2;
        for (j = 0; j < ph[i].p_filesz && j + 1 < (u64)max; j++)
            out[j] = (char)b[ph[i].p_offset + j];
        out[j] = 0;
        return 0;
    }
    return -3; /* statik: interp yok */
}

/* 1=calistirilabilir yigin gerekli, 0=NX yigin. */
int progheader64_stack_exec(const void *buf, u64 len) {
    const unsigned char *b = (const unsigned char *)buf;
    const Elf64_Ehdr *eh;
    const Elf64_Phdr *ph;
    Elf64_Addr entry;
    int i;
    if (elf64_check(buf, len, &entry) != 0) return -1;
    eh = (const Elf64_Ehdr *)buf;
    ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_GNU_STACK64) continue;
        return (ph[i].p_flags & 1) ? 1 : 0; /* PF_X */
    }
    return 0; /* girdi yoksa NX varsayilir */
}
