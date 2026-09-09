#ifndef CHAROS_DRIVERS_THEME_H
#define CHAROS_DRIVERS_THEME_H

#include <stdint.h>
#include <drivers/gfx.h>

/* 18R: Tema & Erişilebilirlik — dark/light palet + HiDPI-lite ölçek.
 * Tüm DE renkleri buradan okunur; dark varsayılanları eski sabit
 * değerlerle birebir aynıdır (önceki aşama testleri etkilenmez). */

#define THEME_DARK  0
#define THEME_LIGHT 1

/* Renk rolleri */
enum theme_role {
    TH_DESKTOP_BG = 0,
    TH_PANEL_BG,
    TH_TASKBAR_BG,
    TH_TASKBAR_TEXT,
    TH_CLOCK_TEXT,
    TH_ICON_LABEL,
    TH_ICON_TERM,
    TH_ICON_FILES,
    TH_ICON_SETTINGS,
    TH_TITLE_FOCUS,
    TH_TITLE_UNFOCUS,
    TH_TITLE_FG,
    TH_BORDER,
    TH_WINDOW_BG,
    TH_ROLE_COUNT
};

int  theme_init(void);              /* 0 hazır (dark varsayılan). */
int  theme_get(void);               /* THEME_DARK/LIGHT */
int  theme_set(int t);              /* 0 ok, -1 geçersiz */
int  theme_toggle(void);            /* yeni tema id'si */
uint32_t theme_color(enum theme_role r);
/* HiDPI-lite: 1 = 100%, 2 = 200%. Ölçekli metin için theme_text kullan. */
int  theme_set_hidpi(int scale);    /* 1/2, 0 ok */
int  theme_ui_scale(void);
/* Uygulama: masaüstü + tüm çerçeveleri yeniden çiz + birleştir */
void theme_apply(void);
/* Ölçekli metin (yükseklik 8*scale). Dönüş: genişlik. */
int  theme_text(struct gfx_surface* s, int x, int y, const char* str,
               uint32_t color);

int  theme_selftest(void);          /* toggle/apply/hidpi doğrula. 0 PASS. */

#endif
