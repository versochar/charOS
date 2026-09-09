#ifndef CHAROS_DRIVERS_APPS_H
#define CHAROS_DRIVERS_APPS_H

/* 18S: Örnek uygulamalar — metin editörü + görüntüleyici + ayarlar.
 * Hepsi kendi WM penceresinde; launcher menü exec isimlerini çözer. */

int apps_init(void);       /* 3 pencere + menü kayıtları. 0 hazır. */
int apps_selftest(void);   /* editör/viewer/ayarlar/launcher. 0 PASS. */

/* Başlatıcı: exec -> pencereyi odaklar. Dönüş: pencere id / -1 yok. */
int apps_launch(const char* exec);

/* Pencere kimlikleri */
int edit_window_id(void);
int view_window_id(void);
int settings_window_id(void);

/* Metin editörü: 16 satır x 64 sütun, chfs destekli */
int edit_open(const char* path);   /* dosyayı tampona al (yoksa boş) */
int edit_set_line(int row, const char* text);  /* 0 ok */
int edit_get_line(int row, char* out, int max);/* 0 ok */
int edit_save(void);               /* tamponu dosyaya yaz. Dönüş: bayt. */
int edit_nlines(void);
int edit_dirty(void);
void edit_render(void);

/* Görüntüleyici: yerleşik demo + 1x/2x zoom */
int view_set_zoom(int z);          /* 1/2, 0 ok */
int view_zoom(void);
void view_render(void);

/* Ayarlar: tema + hidpi düğmeleri (widget id 1-4) */
int settings_action(int button_id);/* 0 uygulandı */

#endif
