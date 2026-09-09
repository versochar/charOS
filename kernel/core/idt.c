#include <core/idt.h>
#include <core/pic.h>
#include <stdint.h>

/* IDT tablosu ve pointer - 256 entry */
struct idt_entry idt_entries[IDT_ENTRIES];
struct idt_ptr   idt_ptr_reg;

/* Dışarıdan gelen ISR adresleri (interrupts.s) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);  extern void isr3(void);
extern void isr4(void);  extern void isr5(void);  extern void isr6(void);  extern void isr7(void);
extern void isr8(void);  extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void); extern void isr15(void);
extern void isr16(void); extern void isr17(void); extern void isr18(void); extern void isr19(void);
extern void isr20(void); extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void); extern void isr27(void);
extern void isr28(void); extern void isr29(void); extern void isr30(void); extern void isr31(void);
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);  extern void irq3(void);
extern void irq4(void);  extern void irq5(void);  extern void irq6(void);  extern void irq7(void);
extern void irq8(void);  extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void); extern void irq15(void);

/* Dahili yardımcı */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags)
{
    idt_entries[num].base_low  = base & 0xFFFF;
    idt_entries[num].base_high = (base >> 16) & 0xFFFF;
    idt_entries[num].sel       = sel;
    idt_entries[num].always0   = 0;
    idt_entries[num].flags     = flags;
}

/* IDT'yi yükle (lidt) */
void idt_flush(uint32_t ptr)
{
    asm volatile("lidt (%0)" : : "r"(ptr));
}

/* ISR adres tablosu - daha temiz init için */
static void* isr_addrs[48] = {
    isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
    isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
    isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
    isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31,
    irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
    irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
};

void idt_init(void)
{
    idt_ptr_reg.limit = sizeof(struct idt_entry) * IDT_ENTRIES - 1;
    idt_ptr_reg.base  = (uint32_t)&idt_entries;

    /* Tüm entry'leri sıfırla */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    /* İlk 32 ISR (exception) - interrupt gate, DPL=0, P=1, 0x8E */
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, (uint32_t)isr_addrs[i], 0x08, 0x8E);
    }
    /* IRQ 32-47 (PIC remap) */
    for (int i = 32; i < 48; i++) {
        idt_set_gate(i, (uint32_t)isr_addrs[i], 0x08, 0x8E);
    }
    /* 48-255 aralığını isr_spurious ile doldur (#GP veya #DF olmasın) */
    extern void isr_spurious(void);
    extern void isr_spurious_apic(void);
    for (int i = 48; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, (uint32_t)isr_spurious, 0x08, 0x8E);
    }
    /* APIC Spurious interrupt vektörü 0xFF (255) */
    idt_set_gate(0xFF, (uint32_t)isr_spurious_apic, 0x08, 0x8E);

    /* IDT'yi yükle */
    idt_flush((uint32_t)&idt_ptr_reg);
}

/* 11D: AP trampoline IDT'yi yükleyebilsin diye sarmalayıcılar */
uint16_t idt_get_limit(void) { return idt_ptr_reg.limit; }
uint32_t idt_get_base(void)   { return idt_ptr_reg.base; }