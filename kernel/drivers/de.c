#include <drivers/de.h>
#include <drivers/compositor.h>
#include <drivers/fb.h>
#include <drivers/gfx.h>
#include <drivers/font.h>
#include <drivers/wm.h>
#include <drivers/shell.h>
#include <drivers/term.h>
#include <drivers/fm.h>
#include <drivers/apps.h>
#include <drivers/inr.h>
#include <drivers/theme.h>
#include <drivers/vt.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18T: DE açılış orkestrasyonu. Beklenen hazır bileşen sayısı. */
#define DE_EXPECTED 10

static struct gfx_surface de_fb;

static void de_fb_init(void) {
    de_fb.pixels = fb_back_buffer();
    uint32_t w = 1024, h = 768, bpp = 32;
    fb_get_mode(&w, &h, &bpp);
    if (w < 640 || h < 480) { w = 1024; h = 768; }
    de_fb.w = w; de_fb.h = h;
    de_fb.pitch = fb_pitch_bytes();
    de_fb.bypp = fb_bytes_per_pixel();
}

/* Tüm DE bileşenleri otomatik başlamış mı? Dönüş: hazır sayısı. */
int de_boot(void) {
    int ready = 0;
    if (fb_active()) ready++;                       /* 1: framebuffer */
    {
        int t = comp_create(16, 16);                /* 2: compositor canlı */
        if (t >= 0) { comp_destroy(t); ready++; }
    }
    if (term_window_id() >= 0) ready++;             /* 3: terminal */
    if (fm_window_id() >= 0) ready++;               /* 4: dosya yöneticisi */
    if (edit_window_id() >= 0) ready++;             /* 5: editör */
    if (view_window_id() >= 0) ready++;             /* 6: görüntüleyici */
    if (settings_window_id() >= 0) ready++;         /* 7: ayarlar */
    if (comp_cursor_pos(0, 0)) ready++;             /* 8: imleç görünür */
    if (theme_get() == THEME_DARK) ready++;         /* 9: varsayılan tema */
    if (vt_active() == VT_GUI) ready++;             /* 10: GUI VT aktif */
    return ready;
}

/* Açılış ekranı (doğrudan fb): başlık + ilerleme çubuğu. Dönüş: çubuk rengi. */
static uint32_t de_splash(void) {
    de_fb_init();
    uint32_t W = de_fb.w, H = de_fb.h;
    uint32_t bg = theme_color(TH_DESKTOP_BG);
    gfx_fill_rect(&de_fb, 0, 0, (int)W, (int)H, bg);
    /* Başlık: ortalanmış "CHAROS" (48px vektör) */
    font_text_scaled(&de_fb, (int)(W - 6 * 48) / 2, (int)H / 2 - 70,
                     "CHAROS", 48, 0x00FFFFFFu);
    font_text_scaled(&de_fb, (int)(W - 13 * 16) / 2, (int)H / 2,
                     "DESKTOP READY", 16, 0x00BBBBBBu);
    /* İlerleme çubuğu: çerçeve + tam dolum */
    int bw = 300, bh = 14;
    int bx = (int)(W - (uint32_t)bw) / 2, by = (int)H / 2 + 40;
    gfx_rect(&de_fb, bx, by, bw, bh, 0x00CCCCCCu);
    gfx_fill_rect(&de_fb, bx + 2, by + 2, bw - 4, bh - 4, 0x0000CC44u);
    fb_damage_all(); /* doğrudan arka-tampon yazımı: tamamını kirlet */
    fb_flip();
    return 0x0000CC44u;
}

int de_selftest(void) {
    int ok = 1;

    /* 1) Kare/skip sayaçları: kirli birleşim sayar, boş çağrı saymaz */
    {
        uint32_t f0 = comp_frame_count();
        uint32_t s0 = comp_skip_count();
        comp_full_repaint();   /* kirli -> 1 kare */
        comp_composite();      /* temiz -> 1 skip */
        comp_composite();      /* temiz -> 1 skip */
        if (comp_frame_count() != f0 + 1) ok = 0;
        if (comp_skip_count() != s0 + 2) ok = 0;
    }

    /* 2) Vsync açık sunum: kare artar, ekran doğru kalır */
    {
        uint32_t f0 = comp_frame_count();
        comp_set_vsync(1);
        comp_full_repaint();
        comp_set_vsync(0);
        if (comp_frame_count() != f0 + 1) ok = 0;
    }

    /* 3) Boot denetimi: tüm bileşenler otomatik başlamış olmalı */
    {
        int r = de_boot();
        if (r != DE_EXPECTED) ok = 0;
    }

    /* 4) Açılış ekranı + masaüstüne dönüş */
    {
        uint32_t bar = de_splash();
        uint32_t W = 1024, H = 768, bpp = 32;
        fb_get_mode(&W, &H, &bpp);
        if (W < 640 || H < 480) { W = 1024; H = 768; }
        int bx = (int)(W - 300) / 2, by = (int)H / 2 + 40;
        /* Çubuk içi dolu piksel */
        if ((fb_getpixel_front((uint32_t)(bx + 10), (uint32_t)(by + 7)) &
             0x00FFFFFFu) != (bar & 0x00FFFFFFu)) ok = 0;
        /* Başlık glifi: başlık alanında zemin-dışı piksel olmalı */
        {
            uint32_t tbg = theme_color(TH_DESKTOP_BG) & 0x00FFFFFFu;
            int tx = (int)(W - 6 * 48) / 2, ty = (int)H / 2 - 70;
            int found = 0;
            for (int dy = 0; dy < 48 && !found; dy += 3)
                for (int dx = 0; dx < 6 * 48; dx += 3)
                    if ((fb_getpixel_front((uint32_t)(tx + dx),
                                           (uint32_t)(ty + dy)) &
                         0x00FFFFFFu) != tbg) { found = 1; break; }
            if (!found) ok = 0;
        }
        comp_full_repaint(); /* masaüstünü geri yükle */
        /* Görev çubuğu zemini ekranda (koyu tema) */
        if ((fb_getpixel_front(12, H - 6) & 0x00FFFFFFu) != 0x001A1A1Au) ok = 0;
        /* İmleç geri geldi */
        {
            int cx = 0, cy = 0;
            inr_cursor_pos(&cx, &cy);
            if ((fb_getpixel_front((uint32_t)cx, (uint32_t)cy) &
                 0x00FFFFFFu) != 0x00FFFFFFu) ok = 0;
        }
    }

    if (ok) {
        serial_puts("[18T] perf/integration/vsync/autostart [PASS]\n");
        vga_puts("[18T] perf/integration/vsync/autostart [PASS]\n");
        return 0;
    }
    serial_puts("[18T] perf/integration [FAIL]\n");
    vga_puts("[18T] perf/integration [FAIL]\n");
    return -1;
}
