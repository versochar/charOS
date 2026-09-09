#include <drivers/shell.h>
#include <drivers/theme.h>
#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <drivers/rtc.h>
#include <drivers/fb.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18H: Desktop shell — panel, saat, desktop ikonları */

#define PANEL_H      28
#define ICON_W       48
#define ICON_H       48
#define ICON_GAP     10
#define ICON_LABEL_H 10
#define ICON_TOTAL   (ICON_H + ICON_LABEL_H + ICON_GAP)
#define PANEL_PAD    8

static int panel_id = -1;
static int taskbar_id = -1;
static int desktop_icons_id = -1;
static int desk_on = 0;
static uint32_t init_w, init_h;

void desk_init(void) {
    if (!fb_active()) return;
    init_w = 1024;
    init_h = 768;
    {
        uint32_t mw = 0, mh = 0, mb = 0;
        fb_get_mode(&mw, &mh, &mb);
        if (mw >= 640 && mh >= 480) { init_w = mw; init_h = mh; }
    }

    panel_id = comp_create(init_w, PANEL_H);
    if (panel_id < 0) return;

    taskbar_id = comp_create(init_w - PANEL_PAD * 2, PANEL_H - 4);
    if (taskbar_id < 0) return;

    int di_h = 3 * ICON_TOTAL + 4;
    desktop_icons_id = comp_create(ICON_W + 40, (uint32_t)di_h);
    if (desktop_icons_id < 0) return;

    /* 18M: masaüstü yerleşimi — panel/taskbar altta, ikonlar sol üstte */
    comp_move(panel_id, 0, (int)init_h - PANEL_H);
    comp_move(taskbar_id, PANEL_PAD, (int)init_h - PANEL_H + 2);
    comp_move(desktop_icons_id, 0, 0);

    desk_draw_taskbar();
    desk_draw_icons();
    comp_composite();
    desk_on = 1;

    serial_puts("[18H] shell initialized\n");
    vga_puts("[18H] shell initialized\n");
}

void desk_draw_taskbar(void) {
    if (taskbar_id < 0) return;
    /* 18R: panel + taskbar tema renkleri */
    if (panel_id >= 0) {
        comp_fill(panel_id, theme_color(TH_PANEL_BG));
        comp_damage_all(panel_id);
    }
    comp_fill(taskbar_id, theme_color(TH_TASKBAR_BG));
    comp_text(taskbar_id, 4, 4, "charOS", theme_color(TH_TASKBAR_TEXT));
    desk_clock_update();
    comp_damage_all(taskbar_id);
}

void desk_clock_update(void) {
    if (taskbar_id < 0) return;
    struct rtc_time t;
    rtc_get_time(&t);

    char buf[6];
    buf[0] = '0' + (t.hour / 10);
    buf[1] = '0' + (t.hour % 10);
    buf[2] = ':';
    buf[3] = '0' + (t.minute / 10);
    buf[4] = '0' + (t.minute % 10);
    buf[5] = '\0';

    int tb_w = 0, tb_h = 0;
    comp_get_rect(taskbar_id, 0, 0, &tb_w, &tb_h);
    int cx = tb_w - 6 * 8 - 4;
    if (cx < 0) cx = tb_w / 2;
    comp_fill_rect(taskbar_id, cx - 2, 2, 6 * 8 + 4, 12,
                   theme_color(TH_TASKBAR_BG));
    comp_text(taskbar_id, cx, 4, buf, theme_color(TH_CLOCK_TEXT));
    comp_damage(taskbar_id, cx - 2, 2, 6 * 8 + 4, 12);
}

void desk_draw_icons(void) {
    if (desktop_icons_id < 0) return;
    comp_fill(desktop_icons_id, theme_color(TH_DESKTOP_BG));

    int y = 4;
    comp_fill_rect(desktop_icons_id, 4, y, ICON_W, ICON_H,
                   theme_color(TH_ICON_TERM));
    comp_rect(desktop_icons_id, 4, y, ICON_W, ICON_H, 0x00666666);
    comp_text(desktop_icons_id, 4 + (ICON_W - 8*7)/2, y + ICON_H + 2,
              "TERMINAL", theme_color(TH_ICON_LABEL));

    y += ICON_TOTAL;
    comp_fill_rect(desktop_icons_id, 4, y, ICON_W, ICON_H,
                   theme_color(TH_ICON_FILES));
    comp_rect(desktop_icons_id, 4, y, ICON_W, ICON_H, 0x00666666);
    comp_text(desktop_icons_id, 4 + (ICON_W - 8*4)/2, y + ICON_H + 2,
              "FILES", theme_color(TH_ICON_LABEL));

    y += ICON_TOTAL;
    comp_fill_rect(desktop_icons_id, 4, y, ICON_W, ICON_H,
                   theme_color(TH_ICON_SETTINGS));
    comp_rect(desktop_icons_id, 4, y, ICON_W, ICON_H, 0x00666666);
    comp_text(desktop_icons_id, 4 + (ICON_W - 8*7)/2, y + ICON_H + 2,
              "SETTINGS", theme_color(TH_ICON_LABEL));

    comp_damage_all(desktop_icons_id);
}

int desk_selftest(void) {
    if (!desk_on) {
        serial_puts("[18H] shell test atlandı\n");
        return -1;
    }

    int ok = 1;

    /* Taskbar bg = 0x1A1A1A */
    {
        struct gfx_surface* ts = comp_surface(taskbar_id);
        if (ts) {
            if ((((uint32_t*)ts->pixels)[0] & 0x00FFFFFF) != 0x001A1A1A) ok = 0;
        } else ok = 0;
    }

    /* "charOS" label: mavi piksel (0x00AA00) arama */
    {
        struct gfx_surface* ts = comp_surface(taskbar_id);
        if (ts && ok) {
            int found = 0;
            for (int x = 4; x < 52 && x < (int)ts->w; x++) {
                uint32_t p = ((uint32_t*)ts->pixels)[4 * (ts->pitch / 4) + (uint32_t)x];
                if ((p & 0x00FFFF00) == 0x0000AA00) { found = 1; break; }
            }
            if (!found) ok = 0;
        }
    }

    /* Clock: beyaz piksel (0xFFFFFF) */
    {
        struct gfx_surface* ts = comp_surface(taskbar_id);
        if (ts && ok) {
            int cx = (int)ts->w - 6 * 8 - 4;
            if (cx < 0) cx = (int)ts->w / 2;
            int found = 0;
            for (int x = cx; x < cx + 48 && x < (int)ts->w; x++) {
                uint32_t p = ((uint32_t*)ts->pixels)[4 * (ts->pitch / 4) + (uint32_t)x];
                if ((p & 0x00FFFFFF) == 0x00FFFFFF) { found = 1; break; }
            }
            if (!found) ok = 0;
        }
    }

    /* Desktop icon: TERMINAL içi 0x0000AA44 */
    {
        struct gfx_surface* ds = comp_surface(desktop_icons_id);
        if (ds && ok) {
            /* İç piksel: border=1px, iç = (6,6) */
            uint32_t p = ((uint32_t*)ds->pixels)[6 * (ds->pitch / 4) + 6];
            if ((p & 0x00FFFFFF) != 0x0000AA44) ok = 0;
        } else if (!ds) ok = 0;
    }

    /* Panel yüksekliği 28 */
    {
        int pw = 0, ph = 0;
        if (comp_get_rect(panel_id, 0, 0, &pw, &ph) == 0) {
            if (ph != PANEL_H) ok = 0;
        } else ok = 0;
    }

    if (ok) {
        serial_puts("[18H] shell taskbar/icons/clock [PASS]\n");
        vga_puts("[18H] shell taskbar/icons/clock [PASS]\n");
        return 0;
    }
    serial_puts("[18H] shell [FAIL]\n");
    vga_puts("[18H] shell [FAIL]\n");
    return -1;
}

int desk_panel_id(void) { return panel_id; }
int desk_taskbar_id(void) { return taskbar_id; }
int desk_desktop_id(void) { return desktop_icons_id; }
