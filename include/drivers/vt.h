#ifndef CHAROS_DRIVERS_VT_H
#define CHAROS_DRIVERS_VT_H

#include <stdint.h>

/* 18Q: Çoklu sanal terminal — VT1 GUI masaüstü, VT2-4 framebuffer 80x25
 * metin konsolları. Geçiş: Alt+F1..F4 (hub üzerinden inr yakalar).
 * Metin VT'ler bağımsız ekran tamponuna + yankılı girişe sahiptir. */

#define VT_GUI  1
#define VT_MIN  1
#define VT_MAX  4
#define VT_COLS 80
#define VT_ROWS 25

int  vt_init(void);                        /* 0 hazır. Aktif VT1 olur. */
int  vt_switch(int vt);                    /* 1-4. 0 ok, -1 geçersiz. */
int  vt_active(void);                      /* o anki VT (1-4). */
int  vt_gui_active(void);                  /* GUI görünür mü? (init öncesi 1) */
int  vt_write(int vt, const char* s);      /* metin VT tamponuna yaz. */
int  vt_write_n(int vt, const char* s, int len); /* count-sınırlı yaz (tek render) */
int  vt_putc(int vt, char c);
void vt_input_char(char c);                /* canlı tuş -> aktif metin VT */
int  vt_get_row(int vt, int row, char* out, int max); /* satır metni. */
int  vt_clear(int vt);                     /* ekranı sil. 0 ok. */
void vt_start_task(void);                  /* task sonrası vsh kabuk görevi */
int  vt_selftest(void);                    /* switch/izolasyon/giriş. 0 PASS. */

#endif
