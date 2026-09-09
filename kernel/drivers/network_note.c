#include <drivers/network_note.h>
#include <drivers/serial.h>
#include <drivers/vga.h>

void network_note_display(void) {
    serial_puts("[30E] Network note: WiFi RTL8821CE out of scope\n");
    serial_puts("[30E] Development via serial / slirp loopback\n");
}

int network_note_selftest(void) {
    network_note_display();
    serial_puts("[30E] Network note [PASS]\n");
    vga_puts("[30E] Network note [PASS]\n");
    return 0;
}
