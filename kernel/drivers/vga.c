#include <drivers/vga.h>
#include <stdarg.h>

/* ============================================================
 * I/O port fonksiyonları (inline assembly) - EN BAŞTA TANIMLI
 * ============================================================ */
static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/* ============================================================
 * VGA durum değişkenleri
 * ============================================================ */
static volatile uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int vga_cursor_x = 0;
static int vga_cursor_y = 0;
static uint8_t vga_current_color = 0x0F;  /* Beyaz on siyah */

/* ============================================================
 * vga_make_color: Ön plan ve arka plan renginden attribute byte oluştur
 * ============================================================ */
static inline uint8_t vga_make_color(uint8_t fg, uint8_t bg)
{
    return fg | (bg << 4);
}

/* ============================================================
 * vga_make_entry: Karakter ve renkden 16-bit VGA entry oluştur
 * ============================================================ */
static inline uint16_t vga_make_entry(char c, uint8_t color)
{
    return (uint16_t)c | ((uint16_t)color << 8);
}

/* ============================================================
 * vga_init: VGA'yı başlat, ekranı temizle, imleci konumla
 * ============================================================ */
void vga_init(void)
{
    vga_current_color = vga_make_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_clear();
    vga_enable_cursor(14, 15);  /* Blok imleci */
}

/* ============================================================
 * vga_clear: Ekranı temizle ve imleci (0,0)'a getir
 * ============================================================ */
void vga_clear(void)
{
    uint16_t blank = vga_make_entry(' ', vga_current_color);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    vga_cursor_x = 0;
    vga_cursor_y = 0;
    vga_update_cursor(0, 0);
}

/* ============================================================
 * vga_set_color: Yazma rengini ayarla
 * ============================================================ */
void vga_set_color(uint8_t fg, uint8_t bg)
{
    vga_current_color = vga_make_color(fg, bg);
}

/* ============================================================
 * vga_get_color: Mevcut rengi al
 * ============================================================ */
uint8_t vga_get_color(void)
{
    return vga_current_color;
}

/* ============================================================
 * vga_scroll: Ekranı bir satır yukarı kaydır
 * ============================================================ */
static void vga_scroll(void)
{
    uint16_t blank = vga_make_entry(' ', vga_current_color);

    /* Satırları bir üstüne taşı */
    for (int y = 1; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
        }
    }

    /* Son satırı temizle */
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }

    vga_cursor_y = VGA_HEIGHT - 1;
}

/* ============================================================
 * vga_update_cursor: Donanım imlecini güncelle (port 0x3D4/0x3D5)
 * ============================================================ */
void vga_update_cursor(int x, int y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/* ============================================================
 * vga_enable_cursor: İmleci etkinleştir
 * ============================================================ */
void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

/* ============================================================
 * vga_disable_cursor: İmleci gizle
 * ============================================================ */
void vga_disable_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

/* ============================================================
 * vga_putc: Tek karakter yaz (yeni satır, scroll destekli)
 * ============================================================ */
void vga_putc(char c)
{
    if (c == '\n') {
        vga_cursor_x = 0;
        vga_cursor_y++;
    } else if (c == '\r') {
        vga_cursor_x = 0;
    } else if (c == '\t') {
        vga_cursor_x = (vga_cursor_x + 8) & ~7;
        if (vga_cursor_x >= VGA_WIDTH) {
            vga_cursor_x = 0;
            vga_cursor_y++;
        }
    } else if (c == '\b') {
        if (vga_cursor_x > 0) {
            vga_cursor_x--;
            vga_buffer[vga_cursor_y * VGA_WIDTH + vga_cursor_x] = vga_make_entry(' ', vga_current_color);
        }
    } else {
        vga_buffer[vga_cursor_y * VGA_WIDTH + vga_cursor_x] = vga_make_entry(c, vga_current_color);
        vga_cursor_x++;
    }

    /* Satır sonu kontrolü */
    if (vga_cursor_x >= VGA_WIDTH) {
        vga_cursor_x = 0;
        vga_cursor_y++;
    }

    /* Ekran altına geçerse scroll */
    if (vga_cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }

    vga_update_cursor(vga_cursor_x, vga_cursor_y);
}

/* ============================================================
 * vga_puts: Null-terminated string yaz
 * ============================================================ */
void vga_puts(const char* str)
{
    for (int i = 0; str[i] != '\0'; i++) {
        vga_putc(str[i]);
    }
}

/* ============================================================
 * vga_puthex: 32-bit değeri hex olarak yaz
 * ============================================================ */
void vga_puthex(uint32_t val)
{
    const char* hex_chars = "0123456789ABCDEF";
    char buf[9];
    int i = 7;

    if (val == 0) {
        vga_putc('0');
        return;
    }

    while (val > 0 && i >= 0) {
        buf[i] = hex_chars[val & 0xF];
        val >>= 4;
        i--;
    }

    for (int j = i + 1; j < 8; j++) {
        vga_putc(buf[j]);
    }
}

/* ============================================================
 * vga_putdec: 32-bit değeri decimal olarak yaz
 * ============================================================ */
void vga_putdec(uint32_t val)
{
    if (val == 0) {
        vga_putc('0');
        return;
    }

    char buf[11];
    int i = 0;

    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }

    while (i > 0) {
        vga_putc(buf[--i]);
    }
}

/* ============================================================
 * vga_printf: Basit printf (sadece %c, %s, %d, %x, %p, %% destekli)
 * ============================================================ */
void vga_printf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    for (int i = 0; fmt[i] != '\0'; i++) {
        if (fmt[i] == '%') {
            i++;
            switch (fmt[i]) {
                case 'c':
                    vga_putc((char)va_arg(args, int));
                    break;
                case 's':
                    vga_puts(va_arg(args, const char*));
                    break;
                case 'd':
                    vga_putdec(va_arg(args, uint32_t));
                    break;
                case 'x':
                    vga_puthex(va_arg(args, uint32_t));
                    break;
                case 'p':
                    vga_puts("0x");
                    vga_puthex(va_arg(args, uint32_t));
                    break;
                case '%':
                    vga_putc('%');
                    break;
                default:
                    vga_putc('%');
                    vga_putc(fmt[i]);
                    break;
            }
        } else {
            vga_putc(fmt[i]);
        }
    }

    va_end(args);
}