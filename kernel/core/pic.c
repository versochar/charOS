#include <core/pic.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret; asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port)); return ret;
}
static inline void io_wait(void) {
    /* 0x80 portuna yazmak ~1-4us gecikme sağlar (eski POST) */
    outb(0x80, 0);
}

void pic_remap(uint8_t offset1, uint8_t offset2)
{
    uint8_t a1, a2;
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    outb(PIC1, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC2, ICW1_INIT | ICW1_ICW4); io_wait();
    outb(PIC1_DATA, offset1); io_wait();       /* Master offset */
    outb(PIC2_DATA, offset2); io_wait();       /* Slave offset */
    outb(PIC1_DATA, 0x04); io_wait();          /* Master: slave at IRQ2 */
    outb(PIC2_DATA, 0x02); io_wait();          /* Slave: cascade identity */
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    outb(PIC1_DATA, a1); /* Restore masks */
    outb(PIC2_DATA, a2);
}

void pic_init(void)
{
    /* PIC'leri 32-39 ve 40-47'ye remap et (IRQ 0-15 -> int 32-47) */
    pic_remap(0x20, 0x28);

    /* Tüm IRQ'ları maskele (ileride timer/keyboard açılacak) */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);

    /* Alternatif: sadece slave'i tamamen maskele, master'da gerekli olanlar */
    // outb(PIC1_DATA, 0xFF);
    // outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2, PIC_EOI);
    }
    outb(PIC1, PIC_EOI);
}

void pic_set_mask(uint8_t irq_line)
{
    uint16_t port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t val = inb(port) | (1 << (irq_line % 8));
    outb(port, val);
}

void pic_clear_mask(uint8_t irq_line)
{
    uint16_t port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t val = inb(port) & ~(1 << (irq_line % 8));
    outb(port, val);
}

static uint16_t __pic_get_irq_reg(int ocw3)
{
    outb(PIC1, ocw3);
    outb(PIC2, ocw3);
    return (inb(PIC2) << 8) | inb(PIC1);
}

uint16_t pic_get_irr(void) { return __pic_get_irq_reg(0x0A); }
uint16_t pic_get_isr(void) { return __pic_get_irq_reg(0x0B); }

void pic_disable(void)
{
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}