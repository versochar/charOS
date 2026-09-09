/* 31B: ELF64 dogrulama (UEFI loader tarafinda, Boot Services aktifken) */
#include "elf64.h"

int elf64_check(const void *buf, unsigned long long size, Elf64_Addr *entry_out) {
    const unsigned char *b = (const unsigned char *)buf;
    if (!buf || size < sizeof(Elf64_Ehdr)) return -1;
    if (b[0] != ELFMAG0 || b[1] != ELFMAG1 || b[2] != ELFMAG2 || b[3] != ELFMAG3)
        return -2;
    if (b[4] != ELFCLASS64) return -3;   /* 64-bit class sart */
    if (b[5] != ELFDATA2LSB) return -4;
    const Elf64_Ehdr *eh = (const Elf64_Ehdr *)buf;
    if (eh->e_machine != EM_X86_64) return -5;
    if (eh->e_phoff + (unsigned long long)eh->e_phentsize * eh->e_phnum > size)
        return -6;
    if (entry_out) *entry_out = eh->e_entry;
    /* PT_LOAD segmentleri 2MB hizali mi (31C paging64 huge page ile uyum)? */
    const Elf64_Phdr *ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (int i = 0; i < eh->e_phnum; i++) {
        if (ph[i].p_type != PT_LOAD) continue;
        if (ph[i].p_memsz < ph[i].p_filesz) return -7;
        if (ph[i].p_offset + ph[i].p_filesz > size) return -8;
    }
    return 0;
}
