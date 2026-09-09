#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <drivers/fb.h>
#include <memory/kheap.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18D: Compositor - yüzey listesi (sıra = z), yüzey başına kirli rect,
 * birleşim bölgesini alttan üste opak blit + flip. */

#define COMP_MAX 24 /* 18S: 6 pencere x 2 yüzey + kabuk/menü + pay */

#define C_BLUE   0x000000AAu
#define C_GREEN  0x0000AA00u
#define C_RED    0x00AA0000u
#define C_ORA    0x00FFAA00u
#define C_MAG    0x00AA00FFu

struct comp_surface {
    int used;
    int id;
    int x, y;              /* ekran konumu */
    int visible;
    struct gfx_surface g;  /* 32bpp XRGB piksel */
    /* 18L: animasyon */
    int alpha;             /* 0-255; 255=opak */
    int anim_kind;         /* 0=yok, 1=fade_in, 2=fade_out, 3=slide_in */
    int anim_frames;       /* kalan kare */
    int anim_total;        /* toplam kare */
    int from_x, from_y;    /* slide başlangıcı */
    int target_x, target_y;
    int base_x, base_y;    /* fade öncesi/sonrası kalıcı konum */
};

static struct comp_surface comp_list[COMP_MAX];
static int comp_next_id = 1;
static int comp_on = 0;
static uint32_t comp_bg = 0x00000000u;
static struct gfx_surface comp_back; /* fb arka buffer görünümü */

/* Kalıcı ekran-uzayı damage birleşimi (move/destroy'da konum değişse de korunur) */
static int g_dirty = 0;
static int gx0, gy0, gx1, gy1;

static void dmg_add(int x0, int y0, int x1, int y1) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > (int)comp_back.w) x1 = comp_back.w;
    if (y1 > (int)comp_back.h) y1 = comp_back.h;
    if (x0 >= x1 || y0 >= y1) return;
    if (!g_dirty) { gx0 = x0; gy0 = y0; gx1 = x1; gy1 = y1; g_dirty = 1; return; }
    if (x0 < gx0) gx0 = x0;
    if (y0 < gy0) gy0 = y0;
    if (x1 > gx1) gx1 = x1;
    if (y1 > gy1) gy1 = y1;
}

int comp_init(void) {
    if (comp_on) return 0; /* 18M: idempotent — yüzeyleri silme */
    if (!fb_active()) {
        serial_puts("[18D] fb yok, compositor atlandı\n");
        return -1;
    }
    memset(comp_list, 0, sizeof(comp_list));
    comp_next_id = 1;
    comp_back.pixels = fb_back_buffer();
    uint32_t w, h, bpp;
    fb_get_mode(&w, &h, &bpp);
    comp_back.w = w; comp_back.h = h;
    comp_back.pitch = fb_pitch_bytes();
    comp_back.bypp = fb_bytes_per_pixel();
    comp_on = 1;
    serial_puts("[18D] compositor hazır\n");
    return 0;
}

static struct comp_surface* comp_get(int id) {
    for (int i = 0; i < COMP_MAX; i++)
        if (comp_list[i].used && comp_list[i].id == id) return &comp_list[i];
    return 0;
}

int comp_create(uint32_t w, uint32_t h) {
    if (!comp_on || w == 0 || h == 0) return -1;
    if (w > 1024 || h > 1024) return -1;
    if (w * h > 256 * 1024) return -1; /* 1MB üstü yüzey yok (kheap) */
    for (int i = 0; i < COMP_MAX; i++) {
        if (comp_list[i].used) continue;
        uint8_t* px = (uint8_t*)kmalloc(w * h * 4);
        if (!px) return -1;
        memset(px, 0, w * h * 4);
        comp_list[i].used = 1;
        comp_list[i].id = comp_next_id++;
        comp_list[i].x = comp_list[i].y = 0;
        comp_list[i].visible = 1;
        comp_list[i].alpha = 255;
        comp_list[i].g.pixels = px;
        comp_list[i].g.w = w; comp_list[i].g.h = h;
        comp_list[i].g.pitch = w * 4; comp_list[i].g.bypp = 4;
        dmg_add(comp_list[i].x, comp_list[i].y,
                comp_list[i].x + (int)w, comp_list[i].y + (int)h);
        return comp_list[i].id;
    }
    return -1;
}

void comp_destroy(int id) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    /* Kapanan rect'i doğrudan ekran-uzayında kirlet (altı görünsün) */
    dmg_add(s->x, s->y, s->x + (int)s->g.w, s->y + (int)s->g.h);
    if (s->g.pixels) kfree(s->g.pixels);
    memset(s, 0, sizeof(*s));
}

void comp_move(int id, int x, int y) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    dmg_add(s->x, s->y, s->x + (int)s->g.w, s->y + (int)s->g.h); /* eski konum */
    s->x = x; s->y = y;
    dmg_add(s->x, s->y, s->x + (int)s->g.w, s->y + (int)s->g.h); /* yeni konum */
}

void comp_raise(int id) {
    /* En üste al: kaydı listeden çıkarıp used girdilerin sonuna ekle */
    int idx = -1;
    for (int i = 0; i < COMP_MAX; i++)
        if (comp_list[i].used && comp_list[i].id == id) { idx = i; break; }
    if (idx < 0) return;
    struct comp_surface tmp = comp_list[idx];
    for (int i = idx; i < COMP_MAX - 1; i++) comp_list[i] = comp_list[i + 1];
    memset(&comp_list[COMP_MAX - 1], 0, sizeof(tmp));
    struct comp_surface ordered[COMP_MAX];
    int n = 0;
    for (int i = 0; i < COMP_MAX; i++)
        if (comp_list[i].used) ordered[n++] = comp_list[i];
    ordered[n++] = tmp;
    for (int i = 0; i < COMP_MAX; i++) {
        if (i < n) comp_list[i] = ordered[i];
        else memset(&comp_list[i], 0, sizeof(comp_list[i]));
    }
    comp_damage_all(id);
}

void comp_fill(int id, uint32_t color) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    gfx_fill(&s->g, color);
    comp_damage_all(id);
}

void comp_fill_rect(int id, int x, int y, int w, int h, uint32_t color) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    gfx_fill_rect(&s->g, x, y, w, h, color);
    comp_damage(id, x, y, w, h);
}

void comp_rect(int id, int x, int y, int w, int h, uint32_t color) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    gfx_rect(&s->g, x, y, w, h, color);
    comp_damage(id, x, y, w, h);
}

int comp_resize(int id, uint32_t w, uint32_t h) {
    struct comp_surface* s = comp_get(id);
    if (!s || w == 0 || h == 0) return -1;
    if (w > 1024 || h > 1024 || w * h > 256 * 1024) return -1;
    uint8_t* npx = (uint8_t*)kmalloc(w * h * 4);
    if (!npx) return -1;
    memset(npx, 0, w * h * 4);
    struct gfx_surface ns = {npx, w, h, w * 4, 4};
    /* Eski içeriği kırparak koru */
    gfx_blit(&ns, 0, 0, &s->g, 0, 0, w, h);
    dmg_add(s->x, s->y, s->x + (int)s->g.w, s->y + (int)s->g.h); /* eski alan */
    kfree(s->g.pixels);
    s->g.pixels = npx; s->g.w = w; s->g.h = h; s->g.pitch = w * 4;
    dmg_add(s->x, s->y, s->x + (int)w, s->y + (int)h); /* yeni alan */
    return 0;
}

void comp_set_visible(int id, int v) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    if (v) {
        s->visible = 1;
        comp_damage_all(id);
    } else {
        comp_damage_all(id); /* gizlemeden önce bölgeyi kirlet */
        s->visible = 0;
    }
}

int comp_get_rect(int id, int* x, int* y, int* w, int* h) {
    struct comp_surface* s = comp_get(id);
    if (!s) return -1;
    if (x) *x = s->x;
    if (y) *y = s->y;
    if (w) *w = (int)s->g.w;
    if (h) *h = (int)s->g.h;
    return 0;
}

void comp_text(int id, int x, int y, const char* str, uint32_t color) {
    struct comp_surface* s = comp_get(id);
    if (!s || !str) return;
    gfx_text(&s->g, x, y, str, color);
    int len = strlen(str);
    comp_damage(id, x, y, len * 8, 8);
}

void comp_damage(int id, int x, int y, int w, int h) {
    struct comp_surface* s = comp_get(id);
    if (!s || w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)s->g.w) w = s->g.w - x;
    if (y + h > (int)s->g.h) h = s->g.h - y;
    if (w <= 0 || h <= 0) return;
    dmg_add(s->x + x, s->y + y, s->x + x + w, s->y + y + h);
}

void comp_damage_all(int id) {
    struct comp_surface* s = comp_get(id);
    if (!s) return;
    dmg_add(s->x, s->y, s->x + (int)s->g.w, s->y + (int)s->g.h);
}

void comp_set_bg(uint32_t color) { comp_bg = color; }

struct gfx_surface* comp_surface(int id) {
    struct comp_surface* s = comp_get(id);
    if (!s) return 0;
    return &s->g;
}

/* 18M: alttan üste z-sıralı comp id listesi. Dönüş: yazılan sayı. */
int comp_zlist(int* out, int max) {
    if (!out || max <= 0) return 0;
    int n = 0;
    for (int i = 0; i < COMP_MAX && n < max; i++)
        if (comp_list[i].used) out[n++] = comp_list[i].id;
    return n;
}

/* 18M: yazılım imleci — yüzey harcamadan comp_composite sonunda çizilir. */
#define CUR_W 12
#define CUR_H 12
static int cursor_on = 0;
static int cursor_x = 0, cursor_y = 0;
static const char* cursor_rows[CUR_H] = {
    "W...........",
    "WW..........",
    "WWW.........",
    "WWWW........",
    "WWWWW.......",
    "WWWWWW......",
    "WWWWWWW.....",
    "WWWWWWWW....",
    "WWWWWWWWW...",
    "WWWWWW......",
    "WW.WW.......",
    "W...WW......"
};

static void cursor_damage(void) {
    dmg_add(cursor_x, cursor_y, cursor_x + CUR_W + 1, cursor_y + CUR_H + 1);
}

void comp_cursor_show(int x, int y) {
    cursor_on = 1;
    cursor_x = x; cursor_y = y;
    cursor_damage();
}

void comp_cursor_hide(void) {
    if (!cursor_on) return;
    cursor_damage();
    cursor_on = 0;
}

void comp_cursor_move(int x, int y) {
    if (!cursor_on) { cursor_on = 1; cursor_x = x; cursor_y = y; cursor_damage(); return; }
    cursor_damage(); /* eski konum */
    cursor_x = x; cursor_y = y;
    cursor_damage(); /* yeni konum */
}

int comp_cursor_pos(int* x, int* y) {
    if (x) *x = cursor_x;
    if (y) *y = cursor_y;
    return cursor_on;
}

static void cursor_plot(int x, int y, uint32_t color) {
    if (x < 0 || y < 0 || x >= (int)comp_back.w || y >= (int)comp_back.h) return;
    uint32_t* dp = (uint32_t*)comp_back.pixels;
    dp[(uint32_t)y * (comp_back.pitch / 4) + (uint32_t)x] = color;
}

static void cursor_draw(void) {
    /* Önce gölge (1px sağ-alt), sonra beyaz ok */
    for (int r = 0; r < CUR_H; r++) {
        const char* row = cursor_rows[r];
        for (int c = 0; c < CUR_W; c++) {
            if (row[c] != 'W') continue;
            cursor_plot(cursor_x + c + 1, cursor_y + r + 1, 0xFF000000u);
        }
    }
    for (int r = 0; r < CUR_H; r++) {
        const char* row = cursor_rows[r];
        for (int c = 0; c < CUR_W; c++) {
            if (row[c] != 'W') continue;
            cursor_plot(cursor_x + c, cursor_y + r, 0xFFFFFFFFu);
        }
    }
}

/* 18L: Alfa harmanlamalı blit. Dst altta zaten çizilmiş içeriği korur,
 * src pikseli a (0-255) katsayısıyla üzerine harmanlanır. */
static void blit_alpha(struct gfx_surface* dst, int dx0, int dy0,
                       struct gfx_surface* src, int sx0, int sy0,
                       int w, int h, int a) {
    if (!dst || !src || w <= 0 || h <= 0 || a <= 0) return;
    if (a >= 255) { gfx_blit(dst, dx0, dy0, src, sx0, sy0, w, h); return; }
    uint32_t* sp = (uint32_t*)src->pixels;
    uint32_t* dp = (uint32_t*)dst->pixels;
    int spw = src->pitch / 4;
    int dpw = dst->pitch / 4;
    /* Yüzeyler opak XRGB'dir (alfa byte'ı 0); harmanlama katsayısı a'dır. */
    for (int y = 0; y < h; y++) {
        int sy = sy0 + y, dy = dy0 + y;
        if (sy < 0 || sy >= (int)src->h) continue;
        if (dy < 0 || dy >= (int)dst->h) continue;
        for (int x = 0; x < w; x++) {
            int sx = sx0 + x, dx = dx0 + x;
            if (sx < 0 || sx >= (int)src->w) continue;
            if (dx < 0 || dx >= (int)dst->w) continue;
            uint32_t sc = sp[sy * spw + sx];
            uint32_t dc = dp[dy * dpw + dx];
            int sa = a;
            if (sa >= 255) {
                dp[dy * dpw + dx] = sc;
                continue;
            }
            int r = ((sc >> 16) & 0xFF), g2 = ((sc >> 8) & 0xFF), b = (sc & 0xFF);
            int dr = (dc >> 16) & 0xFF, dg2 = (dc >> 8) & 0xFF, db = dc & 0xFF;
            int ir = 255 - sa;
            r = (r * sa + dr * ir) / 255;
            g2 = (g2 * sa + dg2 * ir) / 255;
            b = (b * sa + db * ir) / 255;
            uint32_t out = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g2 << 8) | (uint32_t)b;
            dp[dy * dpw + dx] = out;
        }
    }
}

/* 18Q: GUI katmanı bayrağı (metin VT aktifken compositor durur) */
static int comp_gui_layer = 1;
void comp_set_gui_active(int v) { comp_gui_layer = v ? 1 : 0; }
int comp_gui_active(void) { return comp_gui_layer; }
/* 18Q: tüm ekranı kirletip birleştir (VT dönüşünde GUI'yi tazele) */
void comp_full_repaint(void) {
    if (!comp_on) return;
    dmg_add(0, 0, (int)comp_back.w, (int)comp_back.h);
    comp_composite();
}

/* 18T: vsync + kare sayaçları (performans zamanlaması) */
static int comp_vsync = 0;
static uint32_t comp_frames = 0;
static uint32_t comp_skips = 0;
void comp_set_vsync(int v) { comp_vsync = v ? 1 : 0; }
uint32_t comp_frame_count(void) { return comp_frames; }
uint32_t comp_skip_count(void) { return comp_skips; }

void comp_composite(void) {
    if (!comp_on) return;
    if (!g_dirty) { comp_skips++; return; }
    if (!comp_gui_layer) return; /* metin VT: kirli bölge korunur, çıkış yok */
    int ux0 = gx0, uy0 = gy0, ux1 = gx1, uy1 = gy1;
    g_dirty = 0;
    /* Zemini boya */
    gfx_fill_rect(&comp_back, ux0, uy0, ux1 - ux0, uy1 - uy0, comp_bg);
    /* Alttan üste (liste sırası) opak/alha blit */
    for (int i = 0; i < COMP_MAX; i++) {
        struct comp_surface* s = &comp_list[i];
        if (!s->used || !s->visible) continue;
        int sx0 = ux0 - s->x, sy0 = uy0 - s->y;
        int sw = ux1 - ux0, sh = uy1 - uy0;
        if (s->alpha < 255)
            blit_alpha(&comp_back, ux0, uy0, &s->g, sx0, sy0, sw, sh, s->alpha);
        else
            gfx_blit(&comp_back, ux0, uy0, &s->g, sx0, sy0, sw, sh);
    }
    if (cursor_on) cursor_draw(); /* 18M: imleç her zaman en üstte */
    /* Kirli-alan flip: boyanan birleşim + imleç karesi taşınır */
    fb_damage((uint32_t)ux0, (uint32_t)uy0, (uint32_t)ux1, (uint32_t)uy1);
    if (cursor_on)
        fb_damage((uint32_t)(cursor_x < 0 ? 0 : cursor_x),
                  (uint32_t)(cursor_y < 0 ? 0 : cursor_y),
                  (uint32_t)(cursor_x + CUR_W + 1),
                  (uint32_t)(cursor_y + CUR_H + 1));
    if (comp_vsync) fb_vblank_wait(); /* 18T: yırtılmasız sunum */
    fb_flip();
    comp_frames++;
}

/* ================= 18L: Animasyonlar ================= */

static void anim_reset(struct comp_surface* s) {
    s->anim_kind = 0;
    s->anim_frames = 0;
    s->anim_total = 0;
}

void comp_fade_in(int id, int frames) {
    struct comp_surface* s = comp_get(id);
    if (!s || frames <= 0) return;
    comp_set_visible(id, 1);
    s->alpha = 0;
    s->base_x = s->x; s->base_y = s->y;
    s->anim_kind = 1;
    s->anim_frames = frames;
    s->anim_total = frames;
    comp_damage_all(id);
}

void comp_fade_out(int id, int frames) {
    struct comp_surface* s = comp_get(id);
    if (!s || frames <= 0) return;
    s->base_x = s->x; s->base_y = s->y;
    s->anim_kind = 2;
    s->anim_frames = frames;
    s->anim_total = frames;
    comp_damage_all(id);
}

void comp_slide_in(int id, int from_x, int from_y, int frames) {
    struct comp_surface* s = comp_get(id);
    if (!s || frames <= 0) return;
    comp_set_visible(id, 1);
    s->from_x = from_x; s->from_y = from_y;
    s->target_x = s->x; s->target_y = s->y;
    s->base_x = s->x; s->base_y = s->y;
    s->x = from_x; s->y = from_y;
    s->alpha = 255;
    s->anim_kind = 3;
    s->anim_frames = frames;
    s->anim_total = frames;
    comp_damage_all(id);
}

static void anim_tick(struct comp_surface* s) {
    if (!s->used || s->anim_kind == 0) return;
    int total = s->anim_total, left = s->anim_frames;
    if (left <= 0) { anim_reset(s); return; }
    int remain = left - 1;                        /* bir kare ilerle */
    switch (s->anim_kind) {
        case 1: { /* fade in */
            int a = 255 - (255 * remain) / total;
            if (a > 255) a = 255;
            s->alpha = a;
            break;
        }
        case 2: { /* fade out */
            int a = (255 * remain) / total;
            if (a < 0) a = 0;
            s->alpha = a;
            break;
        }
        case 3: { /* slide in */
            int tx = s->target_x, ty = s->target_y;
            int fx = s->from_x, fy = s->from_y;
            s->x = fx + (tx - fx) * (total - remain) / total;
            s->y = fy + (ty - fy) * (total - remain) / total;
            s->alpha = 255;
            break;
        }
    }
    comp_damage_all(s->id);
    s->anim_frames = remain;
    if (s->anim_frames <= 0) {
        int kind = s->anim_kind;
        s->alpha = 255;
        if (kind == 2) {
            comp_set_visible(s->id, 0);
        }
        anim_reset(s);
    }
}

int comp_anim_step(void) {
    if (!comp_on) return 0;
    int any = 0;
    for (int i = 0; i < COMP_MAX; i++)
        if (comp_list[i].used && comp_list[i].anim_kind != 0) { any = 1; break; }
    if (!any) return 0;
    for (int i = 0; i < COMP_MAX; i++)
        anim_tick(&comp_list[i]);
    comp_composite();
    return comp_anim_active();
}

int comp_anim_active(void) {
    if (!comp_on) return 0;
    for (int i = 0; i < COMP_MAX; i++)
        if (comp_list[i].used && comp_list[i].anim_kind != 0) return 1;
    return 0;
}

/* 18L: fade/slide animasyon selftest'i. 0 PASS. */
int comp_anim_selftest(void) {    if (!comp_on) {
        serial_puts("[18L] anim selftest atlandı (compositor yok)\n");
        return -1;
    }
    int ok = 1;

    /* Diğer çalışan DE yüzeyleri (terminal vb.) selftest alanını kapatmasın:
     * test süresince gizle, sonra geri getir. */
    int saved_vis[COMP_MAX], saved_ids[COMP_MAX], n_vis = 0;
    for (int i = 0; i < COMP_MAX; i++) {
        if (comp_list[i].used && comp_list[i].visible) {
            saved_vis[n_vis] = 1;
            saved_ids[n_vis] = comp_list[i].id;
            comp_set_visible(comp_list[i].id, 0);
            n_vis++;
        }
    }
    uint32_t saved_bg = comp_bg;
    comp_set_bg(C_BLUE);
    int win = comp_create(120, 80);
    if (win < 0) {
        for (int i = 0; i < n_vis; i++) comp_set_visible(saved_ids[i], saved_vis[i]);
        serial_puts("[18L] yüzey OOM\n");
        return -1;
    }
    comp_move(win, 60, 60);
    comp_fill(win, C_MAG);
    comp_composite();

    /* Fade in: alfa 0 -> 255, ara kare harmanlı olmalı */
    comp_fade_in(win, 8);
    int mid_seen = 0;
    for (int k = 0; k < 8; k++) {
        comp_anim_step();
        uint32_t rgb = fb_getpixel_front(80, 100) & 0x00FFFFFFu;
        if (rgb != C_BLUE && rgb != C_MAG) mid_seen = 1;
    }
    if (!mid_seen) ok = 0;
    if (comp_anim_active()) ok = 0;
    if (fb_getpixel_front(80, 100) != C_MAG) ok = 0;

    /* Slide in: soldan sağa kayarak gelsin */
    comp_move(win, 60, 60);
    comp_slide_in(win, 10, 60, 6);
    int moved = 0;
    for (int k = 0; k < 6; k++) {
        comp_anim_step();
        int x = 0, y = 0, w = 0, h = 0;
        comp_get_rect(win, &x, &y, &w, &h);
        if (x > 10 && x < 60) moved = 1;
    }
    if (!moved) ok = 0;
    int ex = 0, ey = 0, ew = 0, eh = 0;
    comp_get_rect(win, &ex, &ey, &ew, &eh);
    if (ex != 60 || ey != 60) ok = 0;

    /* Fade out: sonunda gizlenmeli, alan zemine dönmeli */
    comp_fade_out(win, 8);
    int dim_seen = 0;
    for (int k = 0; k < 8; k++) {
        comp_anim_step();
        uint32_t rgb = fb_getpixel_front(80, 100) & 0x00FFFFFFu;
        if (rgb != C_BLUE && rgb != C_MAG) dim_seen = 1;
    }
    if (!dim_seen) ok = 0;
    if (fb_getpixel_front(80, 100) != C_BLUE) ok = 0;

    comp_destroy(win);
    comp_set_bg(saved_bg);
    comp_composite();
    for (int i = 0; i < n_vis; i++) comp_set_visible(saved_ids[i], saved_vis[i]);
    comp_composite();

    if (ok) {
        serial_puts("[18L] fade/slide animasyon [PASS]\n");
        vga_puts("[18L] fade/slide animasyon [PASS]\n");
        return 0;
    }
    serial_puts("[18L] animasyon [FAIL]\n");
    vga_puts("[18L] animasyon [FAIL]\n");
    return -1;
}

int comp_selftest(void) {
    if (!comp_on) {
        serial_puts("[18D] selftest atlandı (compositor yok)\n");
        return -1;
    }
    comp_set_bg(C_BLUE);
    int win1 = comp_create(160, 100);
    int win2 = comp_create(80, 60);
    if (win1 < 0 || win2 < 0) {
        serial_puts("[18D] yüzey OOM\n");
        return -1;
    }
    int ok = 1;
    comp_move(win1, 50, 40);
    comp_fill(win1, C_GREEN);
    comp_move(win2, 100, 80);
    comp_fill(win2, C_RED);
    comp_composite();
    if (fb_getpixel_front(60, 50) != C_GREEN) ok = 0;   /* yalnız win1 */
    if (fb_getpixel_front(120, 90) != C_RED) ok = 0;     /* kesişim: win2 üstte */
    /* win1'i üste al -> kesişim yeşile dönmeli */
    comp_raise(win1);
    comp_composite();
    if (fb_getpixel_front(120, 90) != C_GREEN) ok = 0;
    /* win1'i taşı: eski yer win2/zemin, yeni yer yeşil */
    comp_move(win1, 300, 200);
    comp_composite();
    if (fb_getpixel_front(120, 90) != C_RED) ok = 0;
    if (fb_getpixel_front(60, 50) != C_BLUE) ok = 0;
    if (fb_getpixel_front(310, 210) != C_GREEN) ok = 0;
    /* Kapat: her yer zemine dönmeli */
    comp_destroy(win1);
    comp_destroy(win2);
    comp_composite();
    if (fb_getpixel_front(310, 210) != C_BLUE) ok = 0;
    if (fb_getpixel_front(120, 90) != C_BLUE) ok = 0;
    if (ok) {
        serial_puts("[18D] compositor z/damage/move [PASS]\n");
        vga_puts("[18D] compositor z/damage/move [PASS]\n");
        return 0;
    }
    serial_puts("[18D] compositor [FAIL]\n");
    vga_puts("[18D] compositor [FAIL]\n");
    return -1;
}
