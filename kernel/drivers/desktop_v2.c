#include <drivers/desktop_v2.h>
#include <drivers/fb.h>
#include <drivers/gfx.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 29A: 1920x1080 masaüstü v2 - temel skeleton.
 * Gerçek GOP/VESA 1920x1080 modu, pencere yöneticisi, kompozitör
 * ileri aşama (29B-29F). */

int desktop_v2_init(void) {
    serial_puts("[29A] Desktop v2 init: 1920x1080 + HiDPI-lite skeleton\n");
    /* Gerçek GOP modu ayarlama burada yapılır (fb_init + gfx_init).
     * Skeleton'da sadece kayıt yapılır. */
    return 0;
}

void desktop_v2_get_res(uint32_t* w, uint32_t* h, uint32_t* scale) {
    *w = DESK_V2_WIDTH;
    *h = DESK_V2_HEIGHT;
    *scale = DESK_V2_HIDPI_SCALE;
}

int desktop_v2_selftest(void) {
    uint32_t w = 0, h = 0, scale = 0;
    desktop_v2_get_res(&w, &h, &scale);
    serial_puts("[29A] Resolution="); serial_puthex(w);
    serial_puts("x"); serial_puthex(h);
    serial_puts(" scale="); serial_puthex(scale);
    serial_puts("\n");
    if (w == DESK_V2_WIDTH && h == DESK_V2_HEIGHT) {
        serial_puts("[29A] Desktop v2 [PASS]\n");
        vga_puts("[29A] Desktop v2 [PASS]\n");
    } else {
        serial_puts("[29A] Desktop v2 [FAIL]\n");
    }
    return 0;
}
