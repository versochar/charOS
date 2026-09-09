#include <drivers/gfx.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <memory/kheap.h>
#include <string.h>

/* 18C: 2D grafik lib */

/* 8x8 bitmap font: space, ! - . / 0-9 : ? A-Z (43 glif) + 18O'da a-z (26).
 * Ölçeklenebilir vektör-lite + UTF-8 için font.c'ye bak. */
struct glyph { char c; uint8_t r[8]; };
static const struct glyph font[] = {
    {' ',{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'!',{0x20,0x20,0x20,0x20,0x20,0x00,0x20,0x00}},
    {'-',{0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x00}},
    {'.',{0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x30}},
    {'/',{0x08,0x08,0x10,0x10,0x20,0x20,0x40,0x40}},
    {'0',{0x70,0x88,0x98,0xA8,0xC8,0x88,0x70,0x00}},
    {'1',{0x20,0x60,0x20,0x20,0x20,0x20,0x70,0x00}},
    {'2',{0x70,0x88,0x08,0x10,0x20,0x40,0xF8,0x00}},
    {'3',{0xF8,0x08,0x08,0x78,0x08,0x08,0xF8,0x00}},
    {'4',{0x10,0x30,0x30,0x58,0xF8,0x10,0x10,0x00}},
    {'5',{0xF8,0x80,0xF0,0x08,0x08,0x88,0x70,0x00}},
    {'6',{0x78,0x80,0x80,0xF0,0x88,0x88,0x70,0x00}},
    {'7',{0xF8,0x08,0x10,0x10,0x20,0x20,0x20,0x00}},
    {'8',{0x70,0x88,0x88,0x70,0x88,0x88,0x70,0x00}},
    {'9',{0x70,0x88,0x88,0x78,0x08,0x08,0x78,0x00}},
    {':',{0x00,0x00,0x30,0x30,0x00,0x30,0x30,0x00}},
    {'?',{0x70,0x88,0x08,0x10,0x20,0x00,0x20,0x00}},
    {'A',{0x20,0x50,0x88,0x88,0xF8,0x88,0x88,0x00}},
    {'B',{0xF0,0x88,0x88,0xF0,0x88,0x88,0xF0,0x00}},
    {'C',{0x78,0x80,0x80,0x80,0x80,0x80,0x78,0x00}},
    {'D',{0xF0,0x88,0x88,0x88,0x88,0x88,0xF0,0x00}},
    {'E',{0xF8,0x80,0x80,0xF0,0x80,0x80,0xF8,0x00}},
    {'F',{0xF8,0x80,0x80,0xF0,0x80,0x80,0x80,0x00}},
    {'G',{0x78,0x80,0x80,0xB8,0x88,0x88,0x78,0x00}},
    {'H',{0x88,0x88,0x88,0xF8,0x88,0x88,0x88,0x00}},
    {'I',{0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00}},
    {'J',{0x30,0x08,0x08,0x08,0x08,0x88,0x70,0x00}},
    {'K',{0x88,0x90,0xA0,0xC0,0xA0,0x90,0x88,0x00}},
    {'L',{0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0x00}},
    {'M',{0x88,0xD8,0xA8,0xA8,0x88,0x88,0x88,0x00}},
    {'N',{0x88,0xC8,0xC8,0xA8,0x98,0x98,0x88,0x00}},
    {'O',{0x70,0x88,0x88,0x88,0x88,0x88,0x70,0x00}},
    {'P',{0xF0,0x88,0x88,0xF0,0x80,0x80,0x80,0x00}},
    {'Q',{0x70,0x88,0x88,0x88,0xA8,0x90,0x68,0x00}},
    {'R',{0xF0,0x88,0x88,0xF0,0xA0,0x90,0x88,0x00}},
    {'S',{0x78,0x80,0x80,0x70,0x08,0x08,0xF0,0x00}},
    {'T',{0xF8,0x20,0x20,0x20,0x20,0x20,0x20,0x00}},
    {'U',{0x88,0x88,0x88,0x88,0x88,0x88,0x70,0x00}},
    {'V',{0x88,0x88,0x88,0x88,0x88,0x50,0x20,0x00}},
    {'W',{0x88,0x88,0x88,0xA8,0xA8,0xD8,0x88,0x00}},
    {'X',{0x88,0x88,0x50,0x20,0x50,0x88,0x88,0x00}},
    {'Y',{0x88,0x88,0x50,0x20,0x20,0x20,0x20,0x00}},
    {'Z',{0xF8,0x08,0x10,0x20,0x40,0x80,0xF8,0x00}},
    /* 18O: gerçek küçük harfler (x-yüksekliği satır 2-6) */
    {'a',{0x00,0x00,0x70,0x08,0x78,0x88,0x78,0x00}},
    {'b',{0x80,0x80,0xB0,0xC8,0x88,0xC8,0xB0,0x00}},
    {'c',{0x00,0x00,0x78,0x80,0x80,0x80,0x78,0x00}},
    {'d',{0x08,0x08,0x68,0x98,0x88,0x98,0x68,0x00}},
    {'e',{0x00,0x00,0x70,0x88,0xF8,0x80,0x78,0x00}},
    {'f',{0x18,0x20,0x20,0x78,0x20,0x20,0x20,0x00}},
    {'g',{0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x70}},
    {'h',{0x80,0x80,0xB0,0xC8,0x88,0x88,0x88,0x00}},
    {'i',{0x20,0x00,0x60,0x20,0x20,0x20,0x70,0x00}},
    {'j',{0x08,0x00,0x18,0x08,0x08,0x08,0x88,0x70}},
    {'k',{0x80,0x80,0x90,0xA0,0xC0,0xA0,0x90,0x00}},
    {'l',{0x60,0x20,0x20,0x20,0x20,0x20,0x70,0x00}},
    {'m',{0x00,0x00,0xD8,0xA8,0xA8,0x88,0x88,0x00}},
    {'n',{0x00,0x00,0xB0,0xC8,0x88,0x88,0x88,0x00}},
    {'o',{0x00,0x00,0x70,0x88,0x88,0x88,0x70,0x00}},
    {'p',{0x00,0x00,0xF0,0x88,0x88,0xF0,0x80,0x80}},
    {'q',{0x00,0x00,0x78,0x88,0x88,0x78,0x08,0x08}},
    {'r',{0x00,0x00,0xB0,0xC8,0x80,0x80,0x80,0x00}},
    {'s',{0x00,0x00,0x78,0x80,0x70,0x08,0xF0,0x00}},
    {'t',{0x20,0x20,0x78,0x20,0x20,0x20,0x18,0x00}},
    {'u',{0x00,0x00,0x88,0x88,0x88,0x88,0x78,0x00}},
    {'v',{0x00,0x00,0x88,0x88,0x88,0x50,0x20,0x00}},
    {'w',{0x00,0x00,0x88,0x88,0xA8,0xA8,0xD8,0x00}},
    {'x',{0x00,0x00,0x88,0x50,0x20,0x50,0x88,0x00}},
    {'y',{0x00,0x00,0x88,0x88,0x88,0x78,0x08,0x70}},
    {'z',{0x00,0x00,0xF8,0x08,0x30,0x40,0xF8,0x00}},
};
#define FONT_N (sizeof(font) / sizeof(font[0]))

static const uint8_t* glyph_rows(char c) {
    /* 18O: küçük harfler artık kendi gliflerine sahip (tabloya bak) */
    for (unsigned i = 0; i < FONT_N; i++)
        if (font[i].c == c) return font[i].r;
    return 0;
}

int gfx_font_has(char c) { return glyph_rows(c) != 0; }

/* 18O: font motorunun ölçekli yedek çizimi için satır erişimi */
const uint8_t* gfx_glyph_rows(char c) { return glyph_rows(c); }

/* XRGB -> hedef formata piksel yaz */
static void surf_put(struct gfx_surface* s, int x, int y, uint32_t color) {
    if (!s || !s->pixels) return;
    if (x < 0 || y < 0 || x >= (int)s->w || y >= (int)s->h) return;
    uint8_t* p = s->pixels + (uint32_t)y * s->pitch + (uint32_t)x * s->bypp;
    if (s->bypp >= 4) {
        p[0] = color & 0xFF; p[1] = (color >> 8) & 0xFF;
        p[2] = (color >> 16) & 0xFF; p[3] = (color >> 24) & 0xFF;
    } else if (s->bypp == 3) {
        p[0] = color & 0xFF; p[1] = (color >> 8) & 0xFF; p[2] = (color >> 16) & 0xFF;
    } else if (s->bypp == 2) {
        /* XRGB -> RGB565 */
        uint16_t v = ((color >> 19) & 0x1F) << 11 | ((color >> 10) & 0x3F) << 5 |
                     ((color >> 3) & 0x1F);
        p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF;
    } else {
        p[0] = (uint8_t)color;
    }
}

static uint32_t surf_get(const struct gfx_surface* s, int x, int y) {
    if (!s || !s->pixels) return 0;
    if (x < 0 || y < 0 || x >= (int)s->w || y >= (int)s->h) return 0;
    const uint8_t* p = s->pixels + (uint32_t)y * s->pitch + (uint32_t)x * s->bypp;
    if (s->bypp >= 4) return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    if (s->bypp == 3) return p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
    if (s->bypp == 2) {
        uint16_t v = p[0] | ((uint16_t)p[1] << 8);
        return ((v >> 11) & 0x1F) * 255 / 31 << 16 |
               ((v >> 5) & 0x3F) * 255 / 63 << 8 |
               (v & 0x1F) * 255 / 31;
    }
    return p[0];
}

/* 18O: font motoru ve testler için ham piksel erişimi (kırpmalı) */
void gfx_putpixel(struct gfx_surface* s, int x, int y, uint32_t color) {
    surf_put(s, x, y, color);
}

uint32_t gfx_getpixel(const struct gfx_surface* s, int x, int y) {
    return surf_get(s, x, y);
}

void gfx_fill(struct gfx_surface* s, uint32_t color) {
    if (!s) return;
    gfx_fill_rect(s, 0, 0, s->w, s->h, color);
}

void gfx_fill_rect(struct gfx_surface* s, int x, int y, int w, int h, uint32_t color) {
    if (!s || !s->pixels || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)s->w) w = s->w - x;
    if (y + h > (int)s->h) h = s->h - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            surf_put(s, xx, yy, color);
}

void gfx_rect(struct gfx_surface* s, int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    gfx_line(s, x, y, x + w - 1, y, color);
    gfx_line(s, x, y + h - 1, x + w - 1, y + h - 1, color);
    gfx_line(s, x, y, x, y + h - 1, color);
    gfx_line(s, x + w - 1, y, x + w - 1, y + h - 1, color);
}

/* Bresenham (tüm oktantlar) */
void gfx_line(struct gfx_surface* s, int x0, int y0, int x1, int y1, uint32_t color) {
    if (!s || !s->pixels) return;
    int dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int dy = y1 >= y0 ? y1 - y0 : y0 - y1;
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;
    for (;;) {
        surf_put(s, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx) { err += dx; y0 += sy; }
    }
}

static void circle_pts(struct gfx_surface* s, int cx, int cy, int x, int y, uint32_t c) {
    surf_put(s, cx + x, cy + y, c); surf_put(s, cx - x, cy + y, c);
    surf_put(s, cx + x, cy - y, c); surf_put(s, cx - x, cy - y, c);
    surf_put(s, cx + y, cy + x, c); surf_put(s, cx - y, cy + x, c);
    surf_put(s, cx + y, cy - x, c); surf_put(s, cx - y, cy - x, c);
}

/* Midpoint circle */
void gfx_circle(struct gfx_surface* s, int cx, int cy, int r, uint32_t color) {
    if (!s || r < 0) return;
    int x = r, y = 0, err = 0;
    while (x >= y) {
        circle_pts(s, cx, cy, x, y, color);
        y++;
        if (err <= 0) err += 2 * y + 1;
        else { x--; err -= 2 * x + 1; }
    }
}

void gfx_fill_circle(struct gfx_surface* s, int cx, int cy, int r, uint32_t color) {
    if (!s || r < 0) return;
    for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
            if (x * x + y * y <= r * r) surf_put(s, cx + x, cy + y, color);
}

/* Kırpmalı blit: kaynak 32bpp XRGB, hedef yüzeye dönüştürülür */
void gfx_blit(struct gfx_surface* dst, int dx, int dy,
              const struct gfx_surface* src, int sx, int sy, int w, int h) {
    if (!dst || !src || !dst->pixels || !src->pixels) return;
    if (w <= 0 || h <= 0) return;
    /* Kaynak kırp */
    if (sx < 0) { w += sx; dx -= sx; sx = 0; }
    if (sy < 0) { h += sy; dy -= sy; sy = 0; }
    if (sx + w > (int)src->w) w = src->w - sx;
    if (sy + h > (int)src->h) h = src->h - sy;
    /* Hedef kırp */
    if (dx < 0) { w += dx; sx -= dx; dx = 0; }
    if (dy < 0) { h += dy; sy -= dy; dy = 0; }
    if (dx + w > (int)dst->w) w = dst->w - dx;
    if (dy + h > (int)dst->h) h = dst->h - dy;
    if (w <= 0 || h <= 0) return;
    if (dst->bypp == 4 && src->bypp == 4 && dst->pitch == dst->w * 4 &&
        src->pitch == src->w * 4 && sx >= 0 && sy >= 0) {
        /* Hızlı yol: satır memcpy */
        for (int r = 0; r < h; r++)
            memcpy(dst->pixels + ((uint32_t)(dy + r) * dst->pitch + (uint32_t)dx * 4),
                   src->pixels + ((uint32_t)(sy + r) * src->pitch + (uint32_t)sx * 4),
                   (uint32_t)w * 4);
        return;
    }
    /* Genel yol: piksel piksel (kaynak 32bpp okunur) */
    for (int r = 0; r < h; r++) {
        for (int c = 0; c < w; c++) {
            const uint8_t* p = src->pixels + (uint32_t)(sy + r) * src->pitch +
                               (uint32_t)(sx + c) * src->bypp;
            uint32_t col = p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
            surf_put(dst, dx + c, dy + r, col);
        }
    }
}

static void glyph_draw(struct gfx_surface* s, int x, int y, char c,
                       uint32_t fg, int use_bg, uint32_t bg) {
    const uint8_t* rows = glyph_rows(c);
    for (int r = 0; r < 8; r++) {
        for (int col = 0; col < 8; col++) {
            int bit = rows ? ((rows[r] >> (7 - col)) & 1) : 0;
            if (bit) surf_put(s, x + col, y + r, fg);
            else if (use_bg) surf_put(s, x + col, y + r, bg);
        }
    }
}

void gfx_text(struct gfx_surface* s, int x, int y, const char* str, uint32_t color) {
    if (!s || !str) return;
    while (*str) {
        glyph_draw(s, x, y, *str, color, 0, 0);
        x += 8;
        str++;
    }
}

void gfx_text_bg(struct gfx_surface* s, int x, int y, const char* str,
                 uint32_t fg, uint32_t bg) {
    if (!s || !str) return;
    while (*str) {
        glyph_draw(s, x, y, *str, fg, 1, bg);
        x += 8;
        str++;
    }
}

int gfx_selftest(void) {
    /* 64x64 offscreen yüzey (kmalloc) */
    uint8_t* px = (uint8_t*)kmalloc(64 * 64 * 4);
    if (!px) {
        serial_puts("[18C] gfx selftest OOM, atlandı\n");
        return -1;
    }
    struct gfx_surface s = {px, 64, 64, 64 * 4, 4};
    int ok = 1;
    gfx_fill(&s, 0x00000000u);
    /* çizgi: köşegen */
    gfx_line(&s, 0, 0, 63, 63, 0x00FF0000u);
    if (surf_get(&s, 0, 0) != 0x00FF0000u) ok = 0;
    if (surf_get(&s, 63, 63) != 0x00FF0000u) ok = 0;
    if (surf_get(&s, 32, 32) != 0x00FF0000u) ok = 0;
    /* rect outline: içi boş kalmalı */
    gfx_rect(&s, 40, 5, 10, 8, 0x0000FF00u);
    if (surf_get(&s, 40, 5) != 0x0000FF00u) ok = 0;
    if (surf_get(&s, 44, 8) != 0x00000000u) ok = 0;
    /* filled rect */
    gfx_fill_rect(&s, 40, 20, 6, 4, 0x000000FFu);
    if (surf_get(&s, 42, 21) != 0x000000FFu) ok = 0;
    /* circle: en üst nokta */
    gfx_circle(&s, 20, 50, 8, 0x00FFFF00u);
    if (surf_get(&s, 20, 42) != 0x00FFFF00u) ok = 0;
    /* filled circle merkezi */
    gfx_fill_circle(&s, 50, 50, 5, 0x00FF00FFu);
    if (surf_get(&s, 50, 50) != 0x00FF00FFu) ok = 0;
    /* blit: filled rect'i kopyala */
    gfx_blit(&s, 0, 40, &s, 40, 20, 6, 4);
    if (surf_get(&s, 2, 41) != 0x000000FFu) ok = 0;
    /* kırpma: ekran dışı çizgi çökmesin */
    gfx_line(&s, -100, -100, 200, 10, 0x00FFFFFFu);
    gfx_fill_rect(&s, 60, 60, 100, 100, 0x00111111u);
    /* font: 'A' piksel saysın + space boş geçsin */
    gfx_fill(&s, 0x00000000u);
    gfx_text(&s, 0, 0, "A", 0x00FFFFFFu);
    int cnt = 0;
    for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
            if (surf_get(&s, xx, yy)) cnt++;
    if (cnt < 10) ok = 0;
    if (surf_get(&s, 20, 20) != 0) ok = 0;
    gfx_text(&s, 0, 10, " ", 0x00FFFFFFu);
    if (surf_get(&s, 0, 10) != 0) ok = 0;
    if (!gfx_font_has('Z') || gfx_font_has('~')) ok = 0;
    kfree(px);
    if (ok) {
        serial_puts("[18C] gfx2d line/rect/circle/blit/font [PASS]\n");
        vga_puts("[18C] gfx2d line/rect/circle/blit/font [PASS]\n");
        return 0;
    }
    serial_puts("[18C] gfx2d [FAIL]\n");
    vga_puts("[18C] gfx2d [FAIL]\n");
    return -1;
}
