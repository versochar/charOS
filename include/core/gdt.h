#ifndef CHAROS_CORE_GDT_H
#define CHAROS_CORE_GDT_H

#include <stdint.h>

/* GDT segment selector indeksleri */
#define GDT_NULL        0x00
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x1B
#define GDT_USER_DATA   0x23
#define GDT_TSS         0x28   /* BSP (cpu 0) TSS */
#define GDT_TSS_AP_SEL  0x30   /* AP (cpu 1) TSS - tek paylaşılan GDT, 2. TSS slotu */

/* GDT descriptor yapısı (8 byte) */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* TSS (Task State Segment) yapısı */
struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/* Fonksiyon prototipleri */
void gdt_init(void);
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
void tss_init(void);
void tss_set_stack(uint32_t ss0, uint32_t esp0);
void tss_set_stack_cpu(int cpu, uint32_t ss0, uint32_t esp0); /* 13F+SMP: belirli CPU */
uint32_t tss_get_esp0(void);
uint32_t tss_get_esp0_cpu(int cpu);                            /* 13F+SMP */
uint16_t gdt_get_limit(void); /* 11D: AP trampoline */
uint32_t gdt_get_base(void);  /* 11D: AP trampoline */

extern void gdt_flush(void);
extern void tss_flush(uint16_t selector);

#endif /* CHAROS_CORE_GDT_H */