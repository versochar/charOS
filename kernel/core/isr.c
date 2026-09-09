#include <core/isr.h>
#include <core/pic.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

const char* exception_messages[32] = {
    "Division By Zero",          // 0
    "Debug",                     // 1
    "Non Maskable Interrupt",    // 2
    "Breakpoint",                // 3
    "Overflow",                  // 4
    "Bound Range Exceeded",      // 5
    "Invalid Opcode",            // 6
    "Device Not Available",      // 7
    "Double Fault",              // 8
    "Coprocessor Segment Overrun", // 9
    "Invalid TSS",               // 10
    "Segment Not Present",       // 11
    "Stack Segment Fault",       // 12
    "General Protection Fault",  // 13
    "Page Fault",                // 14
    "Reserved",                  // 15
    "x87 Floating Point",        // 16
    "Alignment Check",           // 17
    "Machine Check",             // 18
    "SIMD Floating Point",       // 19
    "Virtualization",            // 20
    "Control Protection",        // 21
    "Reserved",                  // 22
    "Reserved",                  // 23
    "Reserved",                  // 24
    "Reserved",                  // 25
    "Reserved",                  // 26
    "Reserved",                  // 27
    "Reserved",                  // 28
    "Reserved",                  // 29
    "Security Exception",        // 30
    "Reserved"                   // 31
};

static isr_t interrupt_handlers[256] = {0};

void isr_register_handler(uint8_t n, isr_t handler)
{
    interrupt_handlers[n] = handler;
}
void isr_unregister_handler(uint8_t n)
{
    interrupt_handlers[n] = 0;
}

void isr_init(void)
{
    for (int i = 0; i < 256; i++) interrupt_handlers[i] = 0;
}

/* ISR handler - exception'lar için */
void isr_handler(struct registers* regs)
{
    /* Özel handler varsa çağır */
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
        return;
    }

    /* Varsayılan exception yazdırma */
    if (regs->int_no < 32) {
        /* Kasıtlı debug tuzakları (boot'taki "int $3" IDT testi dahil):
         * banner basmadan sessiz geç; davranış değişmez (zaten halt yoktu),
         * sadece log'daki sahte [EXCEPTION] gürültüsü gider. Testin kendisi
         * "[1B] Breakpoint OK!" satırıyla başarıyı zaten raporlar. */
        if (regs->int_no == 3 || regs->int_no == 4) {
            return;
        }
        vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_puts("\n[EXCEPTION] ");
        vga_puts(exception_messages[regs->int_no]);
        vga_puts(" (");
        vga_putdec(regs->int_no);
        vga_puts(") Err=0x");
        vga_puthex(regs->err_code);
        vga_puts(" EIP=0x");
        vga_puthex(regs->eip);
        vga_puts(" CS=0x");
        vga_puthex(regs->cs);
        vga_puts(" EFLAGS=0x");
        vga_puthex(regs->eflags);
        vga_puts("\n");

        serial_puts("\n[EXCEPTION] ");
        serial_puts(exception_messages[regs->int_no]);
        serial_puts(" int="); serial_puthex(regs->int_no);
        serial_puts(" err=0x"); serial_puthex(regs->err_code);
        serial_puts(" eip=0x"); serial_puthex(regs->eip);
        serial_puts(" efl=0x"); serial_puthex(regs->eflags);
        serial_puts("\n");

        /* Page fault ise CR2'yi de yazdır */
        if (regs->int_no == 14) {
            uint32_t cr2;
            asm volatile("mov %%cr2, %0" : "=r"(cr2));
            vga_puts("CR2 (fault addr)=0x"); vga_puthex(cr2); vga_puts("\n");
            serial_puts("CR2=0x"); serial_puthex(cr2); serial_puts("\n");
        }

        /* #DB (1): TF takılıp zincir yapmasın diye TF'yi temizle, DR6'yı
         * sıfırla ve devam et (#BP gibi). Tek seferlik QEMU/kalıtı
         * breakpoint'ler sistemi durdurmasın. */
        if (regs->int_no == 1) {
            regs->eflags &= ~0x100u; /* TF clear */
            asm volatile("mov %0, %%dr6" : : "r"(0));
            asm volatile("mov %0, %%dr7" : : "r"(0));
            return;
        }

        /* Kritik exception'larda halt */
        if (regs->int_no != 3 && regs->int_no != 4) {
            vga_puts("System halted.\n");
            serial_puts("System halted.\n");
            for (;;) asm volatile("cli; hlt");
        }
    } else {
        /* 32+ ama handler yok - spurious */
        vga_puts("[IRQ] Unhandled int "); vga_putdec(regs->int_no); vga_puts("\n");
        serial_puts("[IRQ] Unhandled "); serial_puthex(regs->int_no); serial_puts("\n");
    }
}

/* IRQ handler - PIC interrupt'ları için */
void irq_handler(struct registers* regs)
{
    /* EOI'yi handler'dan ÖNCE gönder - handler schedule gibi task switch yaparsa
       PIC bir sonraki IRQ'yu alabilsin (aksi halde ISR biti set kalır ve timer durur) */
    pic_send_eoi(regs->int_no - 32);

    /* Özel handler varsa çağır */
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}