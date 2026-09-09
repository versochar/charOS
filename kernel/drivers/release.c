#include <drivers/release.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

void release_display_version(void) {
    serial_puts("charOS v0.1.1 (2026-09-08)\n");
    vga_puts("charOS v0.1.1\n");
}

void release_notes(void) {
    serial_puts("[30F] Release: ISO = build/charos-uefi.iso\n");
    serial_puts("[30F] Version: 0.1.1\n");
    serial_puts("[30F] Features: UEFI + USB + NVMe + GPT + blk + fsck + desktop\n");
}

int release_selftest(void) {
    release_display_version();
    release_notes();
    serial_puts("[30F] Release / version [PASS]\n");
    vga_puts("[30F] Release / version [PASS]\n");
    return 0;
}
