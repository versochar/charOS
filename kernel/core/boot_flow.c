#include <core/boot_flow.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>

/* 30A: Açılış akışı skeleton */
int boot_flow_check(void) {
    serial_puts("[30A] Boot flow: GRUB -> kernel -> init -> desktop\n");
    return 0; /* PASS */
}

void boot_start_desktop(void) {
    serial_puts("[30A] Auto-start desktop (skeleton)\n");
}

uint32_t boot_duration_ms(void) {
    return timer_get_ticks() * 10; /* ~10ms / tick */
}
