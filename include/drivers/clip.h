#ifndef CHAROS_DRIVERS_CLIP_H
#define CHAROS_DRIVERS_CLIP_H

/* 18P: Pano (clipboard) + uygulamalar arası sürükle-bırak.
 * Metin panosu (kopyala/yapıştır) + dosya DnD yükü (FM -> terminal/klasör).
 * Ctrl+C/X/V kısayolları inr üzerinden odaklı terminale bağlıdır. */

#define CLIP_MAX 512

int  clip_init(void);                  /* pano + dnd durumu. 0 hazır. */
int  clip_copy(const char* s);         /* metni panoya al. Dönüş: bayt. */
int  clip_paste(char* out, int max);   /* panoyu kopyala. Dönüş: bayt. */
void clip_clear(void);
int  clip_length(void);
int  clip_has_text(void);

/* Dosya sürükle-bırak yükü: kaynak pencere + tam yol */
int  clip_dnd_set_path(const char* path, int src_win); /* 0 kuruldu */
int  clip_dnd_drop_to(int target_win);  /* hedefe bırak: terminal=yol yapıştır,
                                          fm-klasör=taşı. 0 ok. */
void clip_dnd_cancel(void);
int  clip_dnd_active(void);

int  clip_selftest(void);  /* pano + kısayol + dnd doğrula. 0 PASS. */

#endif
