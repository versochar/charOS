/* 34A: ELF64 yukleme duzeni — PT_LOAD araliklari, sayfa hizali.
 * Saf mantik (BS tahsisi loader.c'de); host testi uygun.
 */
#include "elf64.h"

int elf64_layout(const void *buf, unsigned long long size,
                 struct elf64_layout *out, int max, int *count_out) {
    const unsigned char *b = (const unsigned char *)buf;
    const Elf64_Ehdr *eh;
    const Elf64_Phdr *ph;
    Elf64_Addr entry;
    int i, n = 0;
    if (elf64_check(buf, size, &entry) != 0) return -1;
    if (!out || max <= 0 || !count_out) return -2;
    eh = (const Elf64_Ehdr *)buf;
    ph = (const Elf64_Phdr *)(b + eh->e_phoff);
    for (i = 0; i < eh->e_phnum; i++) {
        unsigned long long base, end, pages;
        if (ph[i].p_type != PT_LOAD || !ph[i].p_memsz) continue;
        if (n >= max) return -3; /* cikti sigmaz */
        base = ph[i].p_paddr & ~0xFFFULL;
        end = (ph[i].p_paddr + ph[i].p_memsz + 0xFFFULL) & ~0xFFFULL;
        pages = (end - base) >> 12;
        if (!pages || pages > 0x100000ULL) return -4; /* 4GB cap */
        out[n].paddr = base;
        out[n].memsz = end - base;
        out[n].filesz = ph[i].p_filesz;
        out[n].offset = ph[i].p_offset;
        n++;
    }
    *count_out = n;
    return n ? 0 : -5; /* yuklenecek segment yok */
}
