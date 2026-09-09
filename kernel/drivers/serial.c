#include <drivers/serial.h>
#include <stdarg.h>

/* I/O port helpers */
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static int serial_ready = 0;

int serial_is_ready(void) {
    return serial_ready;
}

void serial_init(void) {
    outb(SERIAL_COM1 + 1, 0x00);    // Disable interrupts
    outb(SERIAL_COM1 + 3, 0x80);    // Enable DLAB
    outb(SERIAL_COM1 + 0, 0x03);    // Divisor low (38400 baud? 115200 with divisor 1, we use 3 = 38400)
    outb(SERIAL_COM1 + 1, 0x00);    // Divisor high
    outb(SERIAL_COM1 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(SERIAL_COM1 + 2, 0xC7);    // Enable FIFO, clear, 14-byte threshold
    outb(SERIAL_COM1 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
    serial_ready = 1;
}

int serial_is_transmit_empty(void) {
    return inb(SERIAL_COM1 + 5) & 0x20;
}

void serial_putc(char c) {
    if (!serial_ready) return;
    int timeout = 1000000;
    while (!serial_is_transmit_empty()) {
        if (--timeout == 0) return;
    }
    outb(SERIAL_COM1, c);
    // QEMU serial with file:serial.log may need newline handling
    if (c == '\n') {
        timeout = 1000000;
        while (!serial_is_transmit_empty()) {
            if (--timeout == 0) return;
        }
        outb(SERIAL_COM1, '\r');
    }
}

void serial_puts(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putc(str[i]);
    }
}

void serial_puthex(uint32_t val) {
    const char* hex = "0123456789ABCDEF";
    char buf[9];
    buf[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    // Trim leading zeros but keep at least one digit
    int start = 0;
    while (buf[start] == '0' && start < 7) start++;
    serial_puts(&buf[start]);
}

void serial_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
                case 'c': serial_putc((char)va_arg(args, int)); break;
                case 's': serial_puts(va_arg(args, const char*)); break;
                case 'x': serial_puthex(va_arg(args, uint32_t)); break;
                case 'd': {
                    uint32_t v = va_arg(args, uint32_t);
                    if (v == 0) serial_putc('0');
                    else {
                        char dec[11]; int idx=0;
                        while (v>0){ dec[idx++]='0'+(v%10); v/=10; }
                        while(idx>0) serial_putc(dec[--idx]);
                    }
                    break;
                }
                case '%': serial_putc('%'); break;
                default: serial_putc('%'); serial_putc(fmt[i]); break;
            }
        } else {
            serial_putc(fmt[i]);
        }
    }
    va_end(args);
}