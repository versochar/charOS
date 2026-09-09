#include <drivers/font.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <memory/kheap.h>
#include <string.h>

/* 18O: Font motoru — UTF-8 + bitmap (TR) + vektör-lite ölçekleme.
 * Tamsayı aritmetik, FPU yok, tablo-dışı bağımlılık yok. */

uint32_t utf8_decode(const char** pp) {
    const unsigned char* s = (const unsigned char*)*pp;
    unsigned char b0 = s[0];
    if (b0 == 0) return 0;
    if (b0 < 0x80) { *pp = (const char*)(s + 1); return b0; }
    uint32_t cp;
    int need;
    uint32_t min;
    if ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; need = 1; min = 0x80; }
    else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; need = 2; min = 0x800; }
    else if ((b0 & 0xF8) == 0xF0) { cp = b0 & 0x07; need = 3; min = 0x10000; }
    else { *pp = (const char*)(s + 1); return FONT_INVALID; }
    for (int i = 1; i <= need; i++) {
        unsigned char b = s[i];
        if (b == 0 || (b & 0xC0) != 0x80) {
            *pp = (const char*)(s + 1);
            return FONT_INVALID;
        }
        cp = (cp << 6) | (b & 0x3F);
    }
    if (cp < min || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        *pp = (const char*)(s + need + 1);
        return FONT_INVALID;
    }
    *pp = (const char*)(s + need + 1);
    return cp;
}

/* --- Kod noktası 8x8 bitmapleri (Türkçe + genişletme) --- */
struct cp_glyph { uint32_t cp; uint8_t r[8]; };
static const struct cp_glyph cp_font[] = {
    {0xE7,{0x00,0x00,0x70,0x80,0x80,0x80,0x70,0x30}}, /* ç */
    {0xC7,{0x78,0x80,0x80,0x80,0x80,0x80,0x78,0x30}}, /* Ç */
    {0x11F,{0x28,0x10,0x78,0x88,0x88,0x78,0x08,0x70}},/* ğ */
    {0x11E,{0x28,0x10,0x78,0x80,0x80,0xB8,0x88,0x78}},/* Ğ */
    {0x131,{0x00,0x00,0x60,0x20,0x20,0x20,0x70,0x00}},/* ı */
    {0x130,{0x20,0x00,0x70,0x20,0x20,0x20,0x70,0x00}},/* İ */
    {0xF6,{0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00}}, /* ö */
    {0xD6,{0x50,0x00,0x70,0x88,0x88,0x88,0x70,0x00}}, /* Ö */
    {0x15F,{0x00,0x00,0x78,0x80,0x70,0x08,0xF0,0x30}},/* ş */
    {0x15E,{0xF8,0x80,0x70,0x08,0x08,0x08,0xF0,0x30}},/* Ş */
    {0xFC,{0x50,0x00,0x88,0x88,0x88,0x88,0x78,0x00}}, /* ü */
    {0xDC,{0x50,0x00,0x88,0x88,0x88,0x88,0x70,0x00}}, /* Ü */
};
#define CP_FONT_N (sizeof(cp_font) / sizeof(cp_font[0]))

static const uint8_t* cp_rows(uint32_t cp) {
    for (unsigned i = 0; i < CP_FONT_N; i++)
        if (cp_font[i].cp == cp) return cp_font[i].r;
    return 0;
}

int font_has_codepoint(uint32_t cp) {
    if (cp < 0x80) return gfx_font_has((char)cp);
    return cp_rows(cp) != 0;
}

static void bitmap_rows_draw(struct gfx_surface* s, int x, int y,
                             const uint8_t* rows, uint32_t fg,
                             int use_bg, uint32_t bg) {
    if (!rows) return;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int bit = (rows[r] >> (7 - c)) & 1;
            if (bit) gfx_putpixel(s, x + c, y + r, fg);
            else if (use_bg) gfx_putpixel(s, x + c, y + r, bg);
        }
    }
}

int font_draw_bitmap(struct gfx_surface* s, int x, int y, uint32_t cp,
                     uint32_t color) {
    if (!s) return 0;
    if (cp < 0x80) {
        char tmp[2];
        tmp[0] = (char)cp; tmp[1] = 0;
        if (!gfx_font_has(tmp[0])) return 0;
        gfx_text(s, x, y, tmp, color);
        return 8;
    }
    const uint8_t* rows = cp_rows(cp);
    if (!rows) return 0;
    bitmap_rows_draw(s, x, y, rows, color, 0, 0);
    return 8;
}

int gfx_text_utf8(struct gfx_surface* s, int x, int y, const char* str,
                  uint32_t color) {
    if (!s || !str) return 0;
    int x0 = x;
    const char* p = str;
    for (;;) {
        uint32_t cp = utf8_decode(&p);
        if (cp == 0) break;
        if (cp == FONT_INVALID) {
            if (gfx_font_has('?')) { char q[2] = {'?', 0}; gfx_text(s, x, y, q, color); }
            x += 8;
            continue;
        }
        if (font_draw_bitmap(s, x, y, cp, color) == 0) {
            if (gfx_font_has('?')) { char q[2] = {'?', 0}; gfx_text(s, x, y, q, color); }
        }
        x += 8;
    }
    return x - x0;
}

int gfx_text_utf8_bg(struct gfx_surface* s, int x, int y, const char* str,
                     uint32_t fg, uint32_t bg) {
    if (!s || !str) return 0;
    int x0 = x;
    const char* p = str;
    for (;;) {
        uint32_t cp = utf8_decode(&p);
        if (cp == 0) break;
        if (cp == FONT_INVALID) cp = '?';
        if (cp < 0x80 && gfx_font_has((char)cp)) {
            char tmp[2];
            tmp[0] = (char)cp; tmp[1] = 0;
            gfx_text_bg(s, x, y, tmp, fg, bg);
        } else {
            const uint8_t* rows = cp_rows(cp);
            if (rows) bitmap_rows_draw(s, x, y, rows, fg, 1, bg);
            else if (gfx_font_has('?')) {
                char q[2] = {'?', 0};
                gfx_text_bg(s, x, y, q, fg, bg);
            }
        }
        x += 8;
    }
    return x - x0;
}

/* --- Vektör-lite: 0..7 ızgarasında çizgi konturları ---
 * Biçim: [nstr, (npts, x0,y0, ...)*]. npts==1 nokta çizer. */
static const int8_t V_A[] = {2, 3,0,7,3,0,6,7, 2,1,5,5,5};
static const int8_t V_B[] = {3, 2,0,0,0,7, 6,0,0,4,0,5,1,5,3,4,4,0,4, 6,0,4,4,4,6,5,6,6,4,7,0,7};
static const int8_t V_C[] = {1, 8,6,1,4,0,2,0,0,2,0,5,2,7,4,7,6,6};
static const int8_t V_D[] = {2, 2,0,0,0,7, 6,0,0,4,0,6,2,6,5,4,7,0,7};
static const int8_t V_E[] = {2, 4,6,0,0,0,0,7,6,7, 2,0,3,4,3};
static const int8_t V_F[] = {2, 3,6,0,0,0,0,7, 2,0,3,4,3};
static const int8_t V_G[] = {1, 10,6,1,4,0,2,0,0,2,0,5,2,7,4,7,6,7,6,4,4,4};
static const int8_t V_H[] = {3, 2,0,0,0,7, 2,6,0,6,7, 2,0,3,6,3};
static const int8_t V_I[] = {3, 2,1,0,5,0, 2,3,0,3,7, 2,1,7,5,7};
static const int8_t V_J[] = {1, 4,5,0,5,6,3,7,1,6};
static const int8_t V_K[] = {3, 2,0,0,0,7, 2,5,0,0,4, 2,2,5,6,7};
static const int8_t V_L[] = {1, 3,0,0,0,7,6,7};
static const int8_t V_M[] = {1, 5,0,7,0,0,3,4,6,0,6,7};
static const int8_t V_N[] = {1, 4,0,7,0,0,6,7,6,0};
static const int8_t V_O[] = {1, 9,2,0,4,0,6,2,6,5,4,7,2,7,0,5,0,2,2,0};
static const int8_t V_P[] = {2, 2,0,7,0,0, 6,0,0,4,0,6,1,6,3,4,4,0,4};
static const int8_t V_Q[] = {2, 9,2,0,4,0,6,2,6,5,4,7,2,7,0,5,0,2,2,0, 2,4,5,6,7};
static const int8_t V_R[] = {3, 2,0,7,0,0, 6,0,0,4,0,6,1,6,3,4,4,0,4, 2,3,4,6,7};
static const int8_t V_S[] = {1, 12,6,1,4,0,2,0,0,1,0,3,2,4,4,4,6,5,6,6,4,7,2,7,0,6};
static const int8_t V_T[] = {2, 2,0,0,6,0, 2,3,0,3,7};
static const int8_t V_U[] = {1, 6,0,0,0,6,2,7,4,7,6,6,6,0};
static const int8_t V_V[] = {1, 3,0,0,3,7,6,0};
static const int8_t V_W[] = {1, 5,0,0,1,7,3,3,5,7,6,0};
static const int8_t V_X[] = {2, 2,0,0,6,7, 2,0,7,6,0};
static const int8_t V_Y[] = {3, 2,0,0,3,3, 2,6,0,3,3, 2,3,3,3,7};
static const int8_t V_Z[] = {1, 4,0,0,6,0,0,7,6,7};
static const int8_t V_0[] = {1, 9,2,0,4,0,6,2,6,5,4,7,2,7,0,5,0,2,2,0};
static const int8_t V_1[] = {2, 3,1,1,3,0,3,7, 2,1,7,5,7};
static const int8_t V_2[] = {1, 6,0,1,2,0,4,0,6,2,0,7,6,7};
static const int8_t V_3[] = {2, 5,0,0,4,0,6,1,5,3,3,3, 5,3,3,5,4,6,5,4,7,0,7};
static const int8_t V_4[] = {2, 2,4,0,4,7, 3,0,1,0,4,6,4};
static const int8_t V_5[] = {1, 8,6,0,0,0,0,3,4,3,6,4,6,6,4,7,0,7};
static const int8_t V_6[] = {1, 10,4,0,2,1,0,3,0,5,2,7,4,7,6,5,6,4,4,3,1,3};
static const int8_t V_7[] = {1, 3,0,0,6,0,3,7};
static const int8_t V_8[] = {2, 9,1,0,5,0,6,1,6,3,5,4,1,4,0,3,0,1,1,0, 9,1,4,5,4,6,5,6,6,5,7,1,7,0,6,0,5,1,4};
static const int8_t V_9[] = {1, 10,2,7,4,6,6,4,6,2,4,0,2,0,0,2,0,3,2,4,5,4};
static const int8_t V_SP[] = {0};
static const int8_t V_DASH[] = {1, 2,1,4,5,4};
static const int8_t V_DOT[] = {1, 1,3,6};
static const int8_t V_SLASH[] = {1, 2,5,0,1,7};
static const int8_t V_COLON[] = {2, 1,3,2, 1,3,5};
static const int8_t V_BANG[] = {2, 2,3,1,3,4, 1,3,6};
static const int8_t V_QM[] = {2, 5,0,0,3,0,6,1,6,3,3,4,3,5, 1,3,6};

struct vector_glyph { uint32_t cp; const int8_t* d; };
static const struct vector_glyph vtab[] = {
    {0x20,V_SP},
    {0x21,V_BANG},{0x2D,V_DASH},{0x2E,V_DOT},{0x2F,V_SLASH},
    {0x30,V_0},{0x31,V_1},{0x32,V_2},{0x33,V_3},{0x34,V_4},
    {0x35,V_5},{0x36,V_6},{0x37,V_7},{0x38,V_8},{0x39,V_9},
    {0x3A,V_COLON},{0x3F,V_QM},
    {0x41,V_A},{0x42,V_B},{0x43,V_C},{0x44,V_D},{0x45,V_E},
    {0x46,V_F},{0x47,V_G},{0x48,V_H},{0x49,V_I},{0x4A,V_J},
    {0x4B,V_K},{0x4C,V_L},{0x4D,V_M},{0x4E,V_N},{0x4F,V_O},
    {0x50,V_P},{0x51,V_Q},{0x52,V_R},{0x53,V_S},{0x54,V_T},
    {0x55,V_U},{0x56,V_V},{0x57,V_W},{0x58,V_X},{0x59,V_Y},
    {0x5A,V_Z},
};
#define VTAB_N (sizeof(vtab) / sizeof(vtab[0]))

static const int8_t* vector_data(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') cp -= 32; /* küçük harf vektörü büyükten */
    for (unsigned i = 0; i < VTAB_N; i++)
        if (vtab[i].cp == cp) return vtab[i].d;
    return 0;
}

int font_vector_draw(struct gfx_surface* s, int x, int y, int size,
                     uint32_t cp, uint32_t color) {
    if (!s || size < 2) return 0;
    const int8_t* d = vector_data(cp);
    if (!d) return 0;
    int nstr = d[0];
    int k = 1;
    int den = 7;
    int span = size - 1;
    for (int st = 0; st < nstr; st++) {
        int npts = d[k++];
        int px = -1, py = -1;
        for (int p = 0; p < npts; p++) {
            int gx = d[k++];
            int gy = d[k++];
            int sx = x + gx * span / den;
            int sy = y + gy * span / den;
            if (p == 0) {
                if (npts == 1) gfx_putpixel(s, sx, sy, color);
                else { px = sx; py = sy; }
            } else {
                gfx_line(s, px, py, sx, sy, color);
                px = sx; py = sy;
            }
        }
    }
    return 1;
}

/* Ölçekli bitmap yedeği (vektörü olmayan kod noktaları için) */
static int scaled_bitmap_draw(struct gfx_surface* s, int x, int y, int size,
                              const uint8_t* rows, uint32_t color) {
    if (!s || !rows || size < 2) return 0;
    for (int ty = 0; ty < size; ty++) {
        int gy = ty * 8 / size;
        for (int tx = 0; tx < size; tx++) {
            int gx = tx * 8 / size;
            if ((rows[gy] >> (7 - gx)) & 1)
                gfx_putpixel(s, x + tx, y + ty, color);
        }
    }
    return 1;
}

static void tofu_draw(struct gfx_surface* s, int x, int y, int size,
                      uint32_t color) {
    if (!s || size < 2) return;
    gfx_rect(s, x, y, size, size, color);
    gfx_line(s, x, y, x + size - 1, y + size - 1, color);
}

int font_text_scaled(struct gfx_surface* s, int x, int y, const char* str,
                     int size, uint32_t color) {
    if (!s || !str || size < 2) return 0;
    int x0 = x;
    const char* p = str;
    for (;;) {
        uint32_t cp = utf8_decode(&p);
        if (cp == 0) break;
        if (cp == FONT_INVALID) { tofu_draw(s, x, y, size, color); x += size; continue; }
        if (font_vector_draw(s, x, y, size, cp, color)) { x += size; continue; }
        /* Vektör yoksa: bitmap büyütme (ASCII gfx tablosu, TR yerel tablo) */
        const uint8_t* rows = 0;
        static uint8_t ascii_rows[8];
        if (cp < 0x80) {
            extern const uint8_t* gfx_glyph_rows(char c);
            const uint8_t* r = gfx_glyph_rows((char)cp);
            if (r) {
                for (int i = 0; i < 8; i++) ascii_rows[i] = r[i];
                rows = ascii_rows;
            }
        } else {
            rows = cp_rows(cp);
        }
        if (rows) scaled_bitmap_draw(s, x, y, size, rows, color);
        else tofu_draw(s, x, y, size, color);
        x += size;
    }
    return x - x0;
}

int font_selftest(void) {
    int ok = 1;

    /* 1) UTF-8 çözümleme */
    {
        const char* p;
        p = "A";            if (utf8_decode(&p) != 0x41) ok = 0;
        p = "\xC3\xA9";    if (utf8_decode(&p) != 0xE9) ok = 0;   /* é U+00E9 */
        p = "\xE2\x82\xBA"; if (utf8_decode(&p) != 0x20BA) ok = 0; /* ₺ */
        p = "\xF0\x9F\x98\x80"; if (utf8_decode(&p) != 0x1F600) ok = 0; /* 4 bayt */
        p = "\xFF";         if (utf8_decode(&p) != FONT_INVALID) ok = 0;
        p = "\xC0\xAF";     if (utf8_decode(&p) != FONT_INVALID) ok = 0; /* aşırı uzun */
        p = "\xE2\x82";     if (utf8_decode(&p) != FONT_INVALID) ok = 0; /* yarım+NUL */
        p = "\xED\xA0\x80"; if (utf8_decode(&p) != FONT_INVALID) ok = 0; /* surrogate */
    }

    /* 2) Kapsama: küçük harf + Türkçe var, '~'/emoji yok */
    if (!font_has_codepoint('a') || !font_has_codepoint('z')) ok = 0;
    if (!font_has_codepoint(0xE7) || !font_has_codepoint(0x11F)) ok = 0;
    if (!font_has_codepoint(0x131) || !font_has_codepoint(0x15F)) ok = 0;
    if (font_has_codepoint('~') || font_has_codepoint(0x1F600)) ok = 0;

    /* 3) Bitmap UTF-8: 'a' ve 'ç' piksel dökmeli, genişlik 8px/karakter */
    {
        uint8_t* px = (uint8_t*)kmalloc(64 * 16 * 4);
        if (!px) return -1;
        struct gfx_surface s = {px, 64, 16, 64 * 4, 4};
        gfx_fill(&s, 0);
        int w = gfx_text_utf8(&s, 0, 0, "a", 0x00FFFFFFu);
        if (w != 8) ok = 0;
        int cnt = 0;
        for (int yy = 0; yy < 8; yy++)
            for (int xx = 0; xx < 8; xx++)
                if (gfx_getpixel(&s, xx, yy)) cnt++;
        if (cnt < 8) ok = 0; /* 'a' içi dolu olmalı */
        gfx_fill(&s, 0);
        w = gfx_text_utf8(&s, 0, 0, "\xC3\xA7", 0x00FFFFFFu); /* ç */
        if (w != 8) ok = 0;
        cnt = 0;
        for (int yy = 0; yy < 8; yy++)
            for (int xx = 0; xx < 8; xx++)
                if (gfx_getpixel(&s, xx, yy)) cnt++;
        if (cnt < 8) ok = 0;
        kfree(px);
    }

    /* 4) Vektör-lite: 16px 'A' tepe pikseli + ilerleme */
    {
        uint8_t* px = (uint8_t*)kmalloc(64 * 32 * 4);
        if (!px) return -1;
        struct gfx_surface s = {px, 64, 32, 64 * 4, 4};
        gfx_fill(&s, 0);
        int w = font_text_scaled(&s, 0, 0, "AB", 16, 0x00FFFFFFu);
        if (w != 32) ok = 0;
        /* A tepesi: gx=3 -> 3*15/7=6, gy=0 -> (6,0) */
        if (gfx_getpixel(&s, 6, 0) == 0) ok = 0;
        /* B sol direği: x=16 (ikinci karakter), üstte piksel olmalı */
        if (gfx_getpixel(&s, 16, 0) == 0) ok = 0;
        kfree(px);
    }

    /* 5) Ölçekli bitmap yedeği + tofu çökmesin */
    {
        uint8_t* px = (uint8_t*)kmalloc(64 * 32 * 4);
        if (!px) return -1;
        struct gfx_surface s = {px, 64, 32, 64 * 4, 4};
        gfx_fill(&s, 0);
        int w = font_text_scaled(&s, 0, 0, "\xC3\xA7", 16, 0x00FFFFFFu);
        if (w != 16) ok = 0;
        int cnt = 0;
        for (int yy = 0; yy < 16; yy++)
            for (int xx = 0; xx < 16; xx++)
                if (gfx_getpixel(&s, xx, yy)) cnt++;
        if (cnt < 10) ok = 0;
        gfx_fill(&s, 0);
        w = font_text_scaled(&s, 0, 0, "\xFF\xFE", 16, 0x00FFFFFFu);
        if (w != 32) ok = 0;
        kfree(px);
    }

    if (ok) {
        serial_puts("[18O] font utf8/vector-lite [PASS]\n");
        vga_puts("[18O] font utf8/vector-lite [PASS]\n");
        return 0;
    }
    serial_puts("[18O] font [FAIL]\n");
    vga_puts("[18O] font [FAIL]\n");
    return -1;
}
