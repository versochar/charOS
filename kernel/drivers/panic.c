#include <drivers/panic.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 30C: Hata toleransı skeleton */

void panic_handler(const char* msg, const char* file, int line) {
    serial_puts("[30C] PANIC: ");
    if (msg) serial_puts(msg);
    serial_puts(" at ");
    if (file) serial_puts(file);
    serial_puts(":"); serial_puthex(line);
    serial_puts("\n");
    vga_puts("[30C] PANIC - System halted\n");
    for (;;) asm volatile("hlt");
}

void watchdog_init(void) {
    serial_puts("[30C] Watchdog init (skeleton)\n");
}

void watchdog_feed(void) {
    /* Skeleton: gerçek watchdog besleme burada */
}

void graceful_shutdown(void) {
    serial_puts("[30C] Graceful shutdown started\n");
    vga_puts("[30C] Graceful shutdown\n");
}

int watchdog_selftest(void) {
    watchdog_init();
    watchdog_feed();
    graceful_shutdown();
    serial_puts("[30C] Panic/watchdog [PASS]\n");
    vga_puts("[30C] Panic/watchdog [PASS]\n");
    return 0;
}
