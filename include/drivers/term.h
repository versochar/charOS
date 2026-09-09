#ifndef CHAROS_DRIVERS_TERM_H
#define CHAROS_DRIVERS_TERM_H

/* 18J: GUI Terminal Emulator — PTY üzerinden TTY.
 * Bir WM penceresi içinde satır tabanlı viewport; PTY (pipe çifti) ile
 * term görevi arasında TTY benzeri komut/çıktı akışı. */

#define TERM_COLS   36     /* görünür sütun (8px glif) */
#define TERM_ROWS   16
#define TERM_VIEW_W (TERM_COLS * 8)
#define TERM_VIEW_H (TERM_ROWS * 10)   /* 8px font + 2 satır aralığı */

#define TERM_BUF_W  80                 /* satır genişliği (scrollback) */
#define TERM_BUF_LINES 64              /* scrollback derinliği */

int  term_init(void);            /* PTY + panorama pencere kur. 0 hazır. */
void term_start_task(void);      /* task sistemi kurulduktan sonra PTY task */
int  term_emit(const char* s);   /* TTY'den çıktı yaz (viewport'a render) */
int  term_input(const char* s);  /* klavye girişini PTY'ye gönder */
int  term_selftest(void);        /* PTY round-trip + render doğrula. 0 PASS. */

/* Tercih: pencerenin hazır olup olmadığı */
int  term_ready(void);

/* 18M: input yönlendirme için pencere kimliği + satır sayısı */
int  term_window_id(void);
int  term_get_line_count(void);
/* 18P: pano kopyası için o anki satır (çıktı NUL-sonlu). Dönüş: uzunluk. */
int  term_get_last_line(char* out, int max);

#endif
