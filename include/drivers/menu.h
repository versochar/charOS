#ifndef CHAROS_DRIVERS_MENU_H
#define CHAROS_DRIVERS_MENU_H

/* 18I: Menü & Launcher — başlat menüsü, uygulama listesi, .desktop-lite.
 * .desktop-lite: satır başına "Name/Exec/Icon" alanları (basit metin formatı). */

#define MENU_APPS_MAX   8
#define MENU_NAME_LEN   24
#define MENU_EXEC_LEN   32
#define MENU_ICON_LEN   8

struct desktop_app {
    char name[MENU_NAME_LEN];
    char exec[MENU_EXEC_LEN];
    char icon[MENU_ICON_LEN];   /* 'T' terminal, 'F' files, 'S' settings ... */
};

int menu_init(void);                 /* başlat menüsü surface'ları kur */
int menu_add_app(const char* name, const char* exec, const char* icon);
void menu_open(void);                /* menüyü göster */
void menu_close(void);
void menu_render(void);              /* içeriği çiz */
int  menu_show(void);                /* 1 açık, 0 kapalı */
int  menu_selftest(void);            /* .desktop-lite parse + render doğrula. 0 PASS. */

/* Diğer yüzeylerle entegrasyon */
int  menu_surface_id(void);
const struct desktop_app* menu_list(int* count);

#endif
