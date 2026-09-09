#include <drivers/theme.h>
#include <drivers/wm.h>
#include <drivers/shell.h>
#include <drivers/compositor.h>
#include <drivers/font.h>
#include <drivers/fb.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <memory/kheap.h>
#include <string.h>

/* 18R: paletler. Dark = eski sabit değerler (18E/18H testleri korunur). */

static const uint32_t pal_dark[TH_ROLE_COUNT] = {
    [TH_DESKTOP_BG]    = 0x00000000u,
    [TH_PANEL_BG]      = 0x00000000u,
    [TH_TASKBAR_BG]    = 0x001A1A1Au,
    [TH_TASKBAR_TEXT]  = 0x0000AAFFu,
    [TH_CLOCK_TEXT]    = 0x00FFFFFFu,
    [TH_ICON_LABEL]    = 0x00BBBBBBu,
    [TH_ICON_TERM]     = 0x0000AA44u,
    [TH_ICON_FILES]    = 0x00AA6600u,
    [TH_ICON_SETTINGS] = 0x004444AAu,
    [TH_TITLE_FOCUS]   = 0x003366CCu,
    [TH_TITLE_UNFOCUS] = 0x00666666u,
    [TH_TITLE_FG]      = 0x00FFFFFFu,
    [TH_BORDER]        = 0x00CCCCCCu,
    [TH_WINDOW_BG]     = 0x00101010u,
};

static const uint32_t pal_light[TH_ROLE_COUNT] = {
    [TH_DESKTOP_BG]    = 0x00B8C8E0u,
    [TH_PANEL_BG]      = 0x00D0D0D0u,
    [TH_TASKBAR_BG]    = 0x00E0E0E0u,
    [TH_TASKBAR_TEXT]  = 0x000000AAu,
    [TH_CLOCK_TEXT]    = 0x00000000u,
    [TH_ICON_LABEL]    = 0x00222222u,
    [TH_ICON_TERM]     = 0x00008833u,
    [TH_ICON_FILES]    = 0x00885500u,
    [TH_ICON_SETTINGS] = 0x00333399u,
    [TH_TITLE_FOCUS]   = 0x002A7FFFu,
    [TH_TITLE_UNFOCUS] = 0x00999999u,
    [TH_TITLE_FG]      = 0x00000000u,
    [TH_BORDER]        = 0x00555555u,
    [TH_WINDOW_BG]     = 0x00F0F0F0u,
};

static int theme_cur = THEME_DARK;
static int theme_on = 0;
static int hidpi_scale = 1;

int theme_init(void) {
    theme_cur = THEME_DARK;
    hidpi_scale = 1;
    theme_on = 1;
    theme_apply();
    serial_puts("[18R] tema hazır (dark)\n");
    return 0;
}

int theme_get(void) { return theme_cur; }

int theme_set(int t) {
    if (t != THEME_DARK && t != THEME_LIGHT) return -1;
    theme_cur = t;
    return 0;
}

int theme_toggle(void) {
    theme_cur = (theme_cur == THEME_DARK) ? THEME_LIGHT : THEME_DARK;
    return theme_cur;
}

uint32_t theme_color(enum theme_role r) {
    if (r < 0 || r >= TH_ROLE_COUNT) return 0;
    if (theme_cur == THEME_LIGHT) return pal_light[r];
    return pal_dark[r];
}

int theme_set_hidpi(int scale) {
    if (scale != 1 && scale != 2) return -1;
    hidpi_scale = scale;
    return 0;
}

int theme_ui_scale(void) { return hidpi_scale; }

void theme_apply(void) {
    comp_set_bg(theme_color(TH_DESKTOP_BG));
    desk_draw_taskbar();
    desk_draw_icons();
    wm_refresh_frames();
    comp_composite();
}

int theme_text(struct gfx_surface* s, int x, int y, const char* str,
               uint32_t color) {
    if (!s || !str) return 0;
    return font_text_scaled(s, x, y, str, 8 * hidpi_scale, color);
}

int theme_selftest(void) {
    if (!theme_on) {
        serial_puts("[18R] tema test atlandı\n");
        return -1;
    }
    int ok = 1;

    /* Geçersiz tema reddedilmeli, hidpi 3 reddedilmeli */
    if (theme_set(7) == 0) ok = 0;
    if (theme_get() != THEME_DARK) ok = 0;
    if (theme_set_hidpi(3) == 0) ok = 0;
    if (theme_ui_scale() != 1) ok = 0;

    /* Light temaya geç + uygula: taskbar zemini açılmalı */
    if (theme_set(THEME_LIGHT) != 0) ok = 0;
    theme_apply();
    {
        struct gfx_surface* ts = comp_surface(desk_taskbar_id());
        if (ts) {
            uint32_t p = ((uint32_t*)ts->pixels)[0] & 0x00FFFFFFu;
            if (p != (THEME_LIGHT == theme_get() ? 0x00E0E0E0u : 0)) ok = 0;
        } else ok = 0;
    }

    /* Geçici pencerenin başlığı light odakta yanmalı */
    int W = wm_create("THEME-T", 120, 80, theme_color(TH_WINDOW_BG));
    if (W < 0) ok = 0;
    else {
        wm_move(W, 720, 500);
        wm_focus(W);
        comp_composite();
        /* Başlık metninden uzak nokta (yerel 100,5) */
        if (fb_getpixel_front(820, 505) != theme_color(TH_TITLE_FOCUS)) ok = 0;
        wm_destroy(W);
        comp_composite();
    }

    /* HiDPI-lite: 2x metin 2x2 blok döker, genişlik 2 katı */
    if (theme_set_hidpi(2) != 0) ok = 0;
    {
        uint8_t* px = (uint8_t*)kmalloc(64 * 32 * 4);
        if (!px) ok = 0;
        else {
            struct gfx_surface s = {px, 64, 32, 64 * 4, 4};
            gfx_fill(&s, 0);
            int w = theme_text(&s, 0, 0, "HI", 0x00FFFFFFu);
            if (w != 2 * 16) ok = 0; /* 2 harf * 16px */
            /* 'H' sol direği tepeden tabana inmeli (vektör 1px) */
            if ((gfx_getpixel(&s, 0, 0) & 0x00FFFFFFu) == 0) ok = 0;
            if ((gfx_getpixel(&s, 0, 15) & 0x00FFFFFFu) == 0) ok = 0;
            kfree(px);
        }
    }
    if (theme_set_hidpi(1) != 0) ok = 0;

    /* Dark'a dön + uygula: eski değerler geri gelmeli */
    if (theme_set(THEME_DARK) != 0) ok = 0;
    theme_apply();
    {
        struct gfx_surface* ts = comp_surface(desk_taskbar_id());
        if (ts) {
            uint32_t p = ((uint32_t*)ts->pixels)[0] & 0x00FFFFFFu;
            if (p != 0x001A1A1Au) ok = 0;
        } else ok = 0;
    }

    if (ok) {
        serial_puts("[18R] tema dark/light/hidpi [PASS]\n");
        vga_puts("[18R] tema dark/light/hidpi [PASS]\n");
        return 0;
    }
    serial_puts("[18R] tema [FAIL]\n");
    vga_puts("[18R] tema [FAIL]\n");
    return -1;
}
