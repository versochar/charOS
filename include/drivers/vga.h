#ifndef CHAROS_DRIVERS_VGA_H
#define CHAROS_DRIVERS_VGA_H

#include <stdint.h>

/* VGA text mode sabitleri */
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

/* Renkler (4-bit foreground + 4-bit background) */
enum vga_color {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
};

/* Fonksiyon prototipleri */
void vga_init(void);
void vga_clear(void);
void vga_putc(char c);
void vga_puts(const char* str);
void vga_puthex(uint32_t val);
void vga_putdec(uint32_t val);
void vga_set_color(uint8_t fg, uint8_t bg);
uint8_t vga_get_color(void);
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
void vga_disable_cursor(void);
void vga_update_cursor(int x, int y);

/* printf benzeri (basit) */
void vga_printf(const char* fmt, ...);

#endif /* CHAROS_DRIVERS_VGA_H */