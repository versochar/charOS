/* 52A: ELF64 parser — baslik + bolum okuma (elf64_check ustu). */
#include "arch/x86_64/longmode.h"
#include "elf64.h"

struct elf64_raw_ehdr {
    unsigned char ident[16];
    u16 type, machine;
    u32 version;
    u64 entry, phoff, shoff;
    u32 flags;
    u16 ehsize, phentsize, phnum, shentsize, shnum, shstrndx;
};

struct elf64_raw_shdr {
    u32 name, type;
    u64 flags, addr, off, size;
    u32 link, info;
    u64 align, entsize;
};

int elfparse64_header(const void *buf, u64 len,
                      struct elfparse64_hdr *out) {
    const struct elf64_raw_ehdr *eh;
    Elf64_Addr entry;
    if (elf64_check(buf, len, &entry) != 0) return -1;
    if (len < sizeof(struct elf64_raw_ehdr)) return -2;
    eh = (const struct elf64_raw_ehdr *)buf;
    if (!out) return -3;
    out->type = eh->type;
    out->machine = eh->machine;
    out->entry = entry;
    out->phoff = eh->phoff;
    out->shoff = eh->shoff;
    out->phnum = eh->phnum;
    out->shnum = eh->shnum;
    return 0;
}

int elfparse64_section(const void *buf, u64 len, int idx, u64 *off,
                       u64 *size, u32 *type) {
    const struct elf64_raw_ehdr *eh;
    const struct elf64_raw_shdr *sh;
    if (!buf || idx < 0) return -1;
    if (len < sizeof(struct elf64_raw_ehdr)) return -2;
    eh = (const struct elf64_raw_ehdr *)buf;
    if (idx >= eh->shnum) return -3;
    if (eh->shoff + (u64)(idx + 1) * sizeof(struct elf64_raw_shdr) > len)
        return -4;
    sh = (const struct elf64_raw_shdr *)((const unsigned char *)buf +
                                         eh->shoff) +
         idx;
    if (off) *off = sh->off;
    if (size) *size = sh->size;
    if (type) *type = sh->type;
    if (sh->off + sh->size < sh->off || sh->off + sh->size > len) return -5;
    return 0;
}
