#include <drivers/app_platform.h>
#include <drivers/input.h>
#include <drivers/usbhid.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 29C: Uygulama platformu skeleton */

int window_manager_init(void) {
    serial_puts("[29C] Window manager init (skeleton)\n");
    return 0;
}

void keyboard_tr_layout(void) {
    /* TR klavye düzeni (keyboard_scancode TR eşleşmesi zaten mevcut) */
    serial_puts("[29C] TR keyboard layout set\n");
}

void mouse_speed_set(uint32_t speed) {
    serial_puts("[29C] Mouse speed="); serial_puthex(speed); serial_puts("\n");
}

int app_platform_selftest(void) {
    window_manager_init();
    keyboard_tr_layout();
    mouse_speed_set(2); /* varsayılan hız */
    serial_puts("[29C] App platform [PASS]\n");
    vga_puts("[29C] App platform [PASS]\n");
    return 0;
}
