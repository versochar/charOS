/* 31B: ELF64 loader destegi (mevcut 32-bit loader.c'yi bozmaz)
 * UEFI loader (zaten 64-bit PE) bu header ile ELF64 kernel imajini
 * dogrulayip entry adresini alir. Gercek yukleme 32C/31D ile birlesir.
 */
#ifndef CHAROS_ELF64_H
#define CHAROS_ELF64_H

typedef unsigned short Elf64_Half;
typedef unsigned int Elf64_Word;
typedef unsigned long long Elf64_Addr;
typedef unsigned long long Elf64_Off;
typedef unsigned long long Elf64_Xword;

#define EI_NIDENT 16
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define ET_EXEC 2
#define EM_X86_64 62
#define PT_LOAD 1
#define PF_R 4
#define PF_W 2
#define PF_X 1

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    Elf64_Half e_type;
    Elf64_Half e_machine;
    Elf64_Word e_version;
    Elf64_Addr e_entry;
    Elf64_Off e_phoff;
    Elf64_Off e_shoff;
    Elf64_Word e_flags;
    Elf64_Half e_ehsize;
    Elf64_Half e_phentsize;
    Elf64_Half e_phnum;
    Elf64_Half e_shentsize;
    Elf64_Half e_shnum;
    Elf64_Half e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    Elf64_Word p_type;
    Elf64_Word p_flags;
    Elf64_Off p_offset;
    Elf64_Addr p_vaddr;
    Elf64_Addr p_paddr;
    Elf64_Xword p_filesz;
    Elf64_Xword p_memsz;
    Elf64_Xword p_align;
} Elf64_Phdr;

/* 0 = ok, <0 = hata kodu */
int elf64_check(const void *buf, unsigned long long size, Elf64_Addr *entry_out);

/* 34A: yukleme duzeni — her PT_LOAD icin sayfa-hizali aralik.
 * out en fazla max girdi alir; count_out doldurulur. 0=ok. */
struct elf64_layout {
    Elf64_Addr paddr;
    Elf64_Xword memsz;
    Elf64_Xword filesz;
    Elf64_Off offset;
};
int elf64_layout(const void *buf, unsigned long long size,
                 struct elf64_layout *out, int max, int *count_out);

#endif
