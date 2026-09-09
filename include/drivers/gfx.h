#ifndef CHAROS_DRIVERS_GFX_H
#define CHAROS_DRIVERS_GFX_H

#include <stdint.h>

/* 18C: 2D grafik lib - yüzey üstünde çizgi/rect/circle/blit/bitmap font.
 * Yüzeyler 32-bit XRGB; hedef farklı bpp ise blit dönüştürür. */

struct gfx_surface {
    uint8_t* pixels;
    uint32_t w, h, pitch;  /* pitch: satır baytı */
    uint8_t bypp;
};

void gfx_fill(struct gfx_surface* s, uint32_t color);
void gfx_line(struct gfx_surface* s, int x0, int y0, int x1, int y1, uint32_t color);
void gfx_rect(struct gfx_surface* s, int x, int y, int w, int h, uint32_t color);
void gfx_fill_rect(struct gfx_surface* s, int x, int y, int w, int h, uint32_t color);
void gfx_circle(struct gfx_surface* s, int cx, int cy, int r, uint32_t color);
void gfx_fill_circle(struct gfx_surface* s, int cx, int cy, int r, uint32_t color);
/* Kırpmalı kopya (kaynak hep 32bpp varsayılır, hedef dönüştürülür) */
void gfx_blit(struct gfx_surface* dst, int dx, int dy,
              const struct gfx_surface* src, int sx, int sy, int w, int h);
/* Bitmap font (8x8: space, 0-9, A-Z, a-z, -.:/!?), şeffaf zemin.
 * gfx_text bayt-odaklıdır (konum uyumu korunur); UTF-8 için font.h'ye bak. */
void gfx_text(struct gfx_surface* s, int x, int y, const char* str, uint32_t color);
void gfx_text_bg(struct gfx_surface* s, int x, int y, const char* str,
                 uint32_t fg, uint32_t bg);
int gfx_font_has(char c);
const uint8_t* gfx_glyph_rows(char c); /* 18O: ham 8 bayt satır (0 yok) */
/* 18O: kırpmalı ham piksel erişimi */
void gfx_putpixel(struct gfx_surface* s, int x, int y, uint32_t color);
uint32_t gfx_getpixel(const struct gfx_surface* s, int x, int y);

int gfx_selftest(void);  /* offscreen yüzeyde tüm primitifler. 0 PASS. */

#endif
