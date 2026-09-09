/* 31G: GDT64 + TSS64 skeleton (boot64.s tablosunu kullanir) */
#include "arch/x86_64/longmode.h"

extern u64 gdt64_table[];
extern struct { u16 limit; u64 base; } gdt64_ptr;

struct tss64 {
    u32 reserved0;
    u64 rsp0, rsp1, rsp2;
    u64 reserved1;
    u64 ist[7];
    u64 reserved2;
    u16 reserved3;
    u16 iomap;
} __attribute__((packed));
static struct tss64 tss64_entry;

void gdt64_load(void) {
    __asm__ volatile("lgdt %0" :: "m"(gdt64_ptr) : "memory");
    /* long mode'da DS/ES/SS reload yeterli; CS zaten 0x08 */
    __asm__ volatile(
        "mov $0x10, %%ax; mov %%ax, %%ds; mov %%ax, %%es; mov %%ax, %%ss"
        ::: "rax", "memory");
}

void tss64_init(void) {
    for (unsigned i = 0; i < sizeof(tss64_entry); i++)
        ((unsigned char *)&tss64_entry)[i] = 0;
    /* Gercek TR load 34x'te (GDT'ye TSS descriptor eklenince). Skeleton: sifirla. */
}
