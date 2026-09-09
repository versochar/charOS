#ifndef CHAROS_DRIVERS_FONT_H
#define CHAROS_DRIVERS_FONT_H

#include <stdint.h>
#include <drivers/gfx.h>

/* 18O: Font motoru — TrueType-lite + UTF-8.
 * - UTF-8 çözümleyici (1-4 bayt, katı: aşırı-uzun/yarım diziler reddedilir).
 * - 8x8 bitmap: ASCII + küçük harf + Türkçe (çÇğĞıİöÖşŞüÜ).
 * - Vektör-lite: 0..7 ızgarasında çizgi konturları, tamsayı ölçekleme
 *   (stb_truetype tarzı outline->raster fikrinin kernelsel minimal hali).
 * gfx_text bayt-odaklı kalır; yeni kod UTF-8 API'leri kullanır. */

#define FONT_INVALID 0xFFFDu

/* *pp'yi ilerletip kod noktasını döndürür (0=NUL, geçersizde FONT_INVALID). */
uint32_t utf8_decode(const char** pp);
/* Kod noktası bitmap veya vektör olarak çizilebilir mi? */
int font_has_codepoint(uint32_t cp);
/* 8x8 bitmap çiz (1 çizildi, 0 yok->'?' çizilmedi). Dönüş: genişlik (8/0). */
int font_draw_bitmap(struct gfx_surface* s, int x, int y, uint32_t cp,
                     uint32_t color);
/* UTF-8 metin, 8px ilerleme. Dönüş: toplam genişlik. */
int gfx_text_utf8(struct gfx_surface* s, int x, int y, const char* str,
                  uint32_t color);
int gfx_text_utf8_bg(struct gfx_surface* s, int x, int y, const char* str,
                     uint32_t fg, uint32_t bg);
/* Vektör-lite: size px yükseklikte outline çiz (1 ok, 0 vektör yok). */
int font_vector_draw(struct gfx_surface* s, int x, int y, int size,
                     uint32_t cp, uint32_t color);
/* Ölçekli UTF-8 metin (vektör varsa outline, yoksa bitmap büyütme/tofu).
 * Dönüş: toplam genişlik (boyut*karakter). */
int font_text_scaled(struct gfx_surface* s, int x, int y, const char* str,
                     int size, uint32_t color);

int font_selftest(void);  /* decode + kapsama + bitmap + vektör. 0 PASS. */

#endif
