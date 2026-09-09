#ifndef CHAROS_DRIVERS_DESK_H
#define CHAROS_DRIVERS_DESK_H

/* 18H: Desktop shell — taskbar, panel, saat, desktop ikonları.
 * Panel: ekran altı, sol=charOS etiketi, orta=saat, sağ=ikon kutucukları.
 * Desktop ikonları: sol üstte dikey liste. */

void desk_init(void);          /* panel + desktop ikonlarını oluştur */
void desk_draw_taskbar(void);  /* taskbar'ı çiz (saat dahil) */
void desk_draw_icons(void);    /* desktop ikonlarını çiz */
void desk_clock_update(void);  /* saati yenile (RTC) */
int  desk_selftest(void);      /* saat formatı + panel boyutu doğrula. 0 PASS. */

/* Selftest dc Surface ID'leri (selftest içinde erişim) */
int desk_panel_id(void);       /* panel comp surface id */
int desk_taskbar_id(void);     /* taskbar comp surface id */
int desk_desktop_id(void);     /* desktop icons comp surface id */

#endif
