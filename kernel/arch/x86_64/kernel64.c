/* 31E: 64-bit kernel_main prototipi
 * 32-bit kernel'e dokunmaz. UEFI loader -> boot64.s -> buraya.
 * Hedef: 2GB RAM / 512MB VRAM / 4 CPU'da serial+VGA banner, sonra halt.
 */
#include "arch/x86_64/longmode.h"

static inline unsigned char mmio_inb64(unsigned short port) {
    unsigned char r;
    __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}

static inline void mmio_outb64(unsigned short port, unsigned char v) {
    __asm__ volatile("outb %0, %1" :: "a"(v), "Nd"(port));
}

static void vga_puts64(const char *s) {
    volatile unsigned short *vga = (volatile unsigned short *)0xB8000;
    static int pos = 0;
    while (*s) {
        vga[pos % (80 * 25)] = (unsigned short)(0x0F00 | (unsigned char)*s);
        pos++; s++;
    }
}

static void serial_puts64(const char *s) {
    while (*s) {
        /* 1M timeout: gercek donanimda UART yoksa takilma (mevcut fix ile uyumlu) */
        unsigned int t = 1000000;
        while (t-- && !(mmio_inb64(0x3FD) & 0x20)) { __asm__ volatile("pause"); }
        if (mmio_inb64(0x3FD) & 0x20) mmio_outb64(0x3F8, (unsigned char)*s);
        s++;
    }
}

void kernel64_main(void *mbi) {
    (void)mbi;
    serial_puts64("K[31E] charOS-64 prototype\n");
    vga_puts64("charOS-64 31E prototype");
    /* 31F-31I init sirasi (skeleton, gercek driver 32-34'te dolar) */
    gdt64_load();
    idt64_init();
    syscall64_init();
    tss64_init();
    stack_protector64_init();
    serial_puts64("K[31J] 64-bit smoke OK (halt)\n");
    for (;;) { __asm__ volatile("hlt"); }
}
