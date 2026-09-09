#include <drivers/menu.h>
#include <drivers/compositor.h>
#include <drivers/gfx.h>
#include <drivers/fb.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 18I: Menü & Launcher — başlat menüsü, uygulama listesi, .desktop-lite */

#define MENU_W        220
#define MENU_ROW_H    26
#define MENU_TITLE_H  24
#define MENU_PAD      6

static struct desktop_app apps[MENU_APPS_MAX];
static int app_count = 0;
static int menu_id = -1;      /* menü comp surface (WM penceresi değil) */
static int menu_visible = 0;
static int menu_on = 0;
static int menu_h = 0;        /* 18M: panel üstü konum için yükseklik */

int menu_init(void) {
    if (!fb_active()) return -1;

    int h = MENU_TITLE_H + app_count * MENU_ROW_H + app_count * MENU_PAD + MENU_PAD;
    if (h < MENU_TITLE_H + 2 * MENU_ROW_H) h = MENU_TITLE_H + 2 * MENU_ROW_H + MENU_PAD;
    menu_h = h;

    menu_id = comp_create(MENU_W, (uint32_t)h);
    if (menu_id < 0) return -1;

    menu_visible = 0;
    comp_set_visible(menu_id, 0);
    menu_on = 1;
    serial_puts("[18I] menu initialized\n");
    vga_puts("[18I] menu initialized\n");
    return 0;
}

int menu_add_app(const char* name, const char* exec, const char* icon) {
    if (app_count >= MENU_APPS_MAX) return -1;
    struct desktop_app* a = &apps[app_count];
    strncpy(a->name, name, MENU_NAME_LEN - 1); a->name[MENU_NAME_LEN-1] = 0;
    strncpy(a->exec, exec, MENU_EXEC_LEN - 1); a->exec[MENU_EXEC_LEN-1] = 0;
    strncpy(a->icon, icon, MENU_ICON_LEN - 1); a->icon[MENU_ICON_LEN-1] = 0;
    app_count++;

    /* Yeni yükseklik: yüzeyi yeniden boyutlandır */
    if (menu_id >= 0) {
        int h = MENU_TITLE_H + app_count * MENU_ROW_H + app_count * MENU_PAD + MENU_PAD;
        comp_resize(menu_id, MENU_W, (uint32_t)h);
        menu_h = h;
        menu_render();
    }
    return app_count - 1;
}

const struct desktop_app* menu_list(int* count) {
    if (count) *count = app_count;
    return apps;
}

int menu_show(void) { return menu_visible; }

void menu_render(void) {
    if (menu_id < 0) return;
    comp_fill(menu_id, 0x00202020);
    /* Başlık çubuğu */
    comp_fill_rect(menu_id, 0, 0, MENU_W, MENU_TITLE_H, 0x00303030);
    comp_text(menu_id, 8, 7, "START", 0x00FFFFFF);

    /* Uygulama listesi */
    int y = MENU_TITLE_H + MENU_PAD;
    for (int i = 0; i < app_count; i++) {
        struct desktop_app* a = &apps[i];
        /* Satır arka planı */
        comp_fill_rect(menu_id, 4, y, MENU_W - 8, MENU_ROW_H, 0x00282828);
        /* İkon kutusu */
        uint32_t ic = 0x004444AA;
        switch (a->icon[0]) {
        case 'T': ic = 0x0000AA44; break;
        case 'F': ic = 0x00AA6600; break;
        case 'S': ic = 0x004444AA; break;
        default:  ic = 0x00666666; break;
        }
        comp_fill_rect(menu_id, 6, y + 3, 20, 20, ic);
        comp_text(menu_id, 34, y + 8, a->name, 0x00DDDDDD);
        y += MENU_ROW_H + MENU_PAD;
    }
    comp_damage_all(menu_id);
}

void menu_open(void) {
    if (menu_id < 0) return;
    /* 18M: menüyü ekran altında, panel üstünde göster */
    {
        uint32_t mw = 1024, mh = 768, mb = 0;
        fb_get_mode(&mw, &mh, &mb);
        if (mw < 640 || mh < 480) { mw = 1024; mh = 768; }
        int my = (int)mh - 28 - menu_h;
        if (my < 0) my = 0;
        comp_move(menu_id, 0, my);
    }
    comp_set_visible(menu_id, 1);
    menu_visible = 1;
    menu_render();
    comp_raise(menu_id);
    comp_composite();
}

void menu_close(void) {
    if (menu_id < 0) return;
    comp_set_visible(menu_id, 0);
    menu_visible = 0;
    comp_composite();
}

int menu_surface_id(void) { return menu_id; }

/* Selftest */
int menu_selftest(void) {
    if (!menu_on) {
        serial_puts("[18I] menu test atlandı\n");
        return -1;
    }

    int ok = 1;

    /* .desktop-lite parse: 3 uygulama ekle ve listeyi doğrula */
    menu_add_app("Terminal", "sh", "T");
    menu_add_app("Files", "ls", "F");
    menu_add_app("Settings", "settings", "S");

    int n = 0;
    const struct desktop_app* l = menu_list(&n);
    if (n != 3) ok = 0;
    if (ok && strcmp(l[0].name, "Terminal") != 0) ok = 0;
    if (ok && strcmp(l[1].exec, "ls") != 0) ok = 0;
    if (ok && strcmp(l[2].icon, "S") != 0) ok = 0;

    menu_render();

    /* Başlık: START metni beyaz piksel (8,7) */
    if (ok) {
        struct gfx_surface* st = comp_surface(menu_id);
        if (st) {
            int found = 0;
            for (int x = 8; x < 8 + 40 && x < (int)st->w; x++) {
                uint32_t p = ((uint32_t*)st->pixels)[7 * (st->pitch / 4) + (uint32_t)x];
                if ((p & 0x00FFFFFF) == 0x00FFFFFF) { found = 1; break; }
            }
            if (!found) ok = 0;
        } else ok = 0;
    }

    /* Terminal ikonu rengi (ilan satır, iç piksel) */
    if (ok) {
        struct gfx_surface* st = comp_surface(menu_id);
        if (st) {
            /* ilk satır y = title_h + pad + 3, x=6+... iç piksel */
            int y = MENU_TITLE_H + MENU_PAD + 7;
            int x = 6 + 10;
            uint32_t p = ((uint32_t*)st->pixels)[(uint32_t)y * (st->pitch / 4) + (uint32_t)x];
            if ((p & 0x00FFFFFF) != 0x0000AA44) ok = 0;
        }
    }

    /* open/close toggle */
    menu_open();
    if (!menu_show()) ok = 0;
    menu_close();
    if (menu_show()) ok = 0;

    if (ok) {
        serial_puts("[18I] menu/launcher desktop-lite [PASS]\n");
        vga_puts("[18I] menu/launcher desktop-lite [PASS]\n");
        return 0;
    }
    serial_puts("[18I] menu [FAIL]\n");
    vga_puts("[18I] menu [FAIL]\n");
    return -1;
}
