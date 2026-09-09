#ifndef CHAROS_CORE_IDT_H
#define CHAROS_CORE_IDT_H

#include <stdint.h>

/* IDT entry yapısı (8 byte) */
struct idt_entry {
    uint16_t base_low;   /* Handler adres low 16-bit */
    uint16_t sel;        /* Code segment selector (0x08) */
    uint8_t  always0;    /* Her zaman 0 */
    uint8_t  flags;      /* Tip ve attribute */
    uint16_t base_high;  /* Handler adres high 16-bit */
} __attribute__((packed));

/* IDT pointer yapısı (6 byte) - lidt için */
struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

#define IDT_ENTRIES 256

/* Fonksiyonlar */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_flush(uint32_t ptr);
uint16_t idt_get_limit(void); /* 11D: AP trampoline */
uint32_t idt_get_base(void);  /* 11D: AP trampoline */

#endif /* CHAROS_CORE_IDT_H */