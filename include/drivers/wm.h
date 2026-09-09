#ifndef CHAROS_DRIVERS_WM_H
#define CHAROS_DRIVERS_WM_H

#include <stdint.h>

/* 18E: Window Manager - çerçeve, odak, sürükle/yeniden boyutlandır, minimize.
 * 18N: her pencere iki ayrı comp yüzeyine sahiptir — frame (w x h,
 * border+title) ve client (w-2 x h-TITLE_H, içerik). İstemci çizimleri
 * çerçeveye taşmaz; odak değişimi istemciyi silmez. */

#define WM_BORDER   1
#define WM_TITLE_H  11  /* border(1) + 8px font + padding */

int wm_init(void);  /* 0 hazır, -1 compositor yok */
int wm_create(const char* title, int w, int h, uint32_t client_bg);
void wm_destroy(int id);
void wm_focus(int id);
int wm_focused(void);
void wm_move(int id, int x, int y);
/* Sürükleme: başlangıçta tutamaç farkı alınır, sonra hedefe taşınır */
void wm_drag_start(int id, int x, int y);
void wm_drag_to(int x, int y);
int wm_resize(int id, int w, int h);
void wm_minimize(int id);
void wm_restore(int id);
int wm_selftest(void);  /* focus/drag/resize/minimize. 0 PASS. */
int wm_get_comp(int id); /* 18G: istemci comp surface id (-1 hata) */
int wm_get_frame(int id); /* 18N: çerçeve comp surface id (-1 hata) */
int wm_surface_selftest(void); /* 18N: frame/client ayrımı. 0 PASS. */
void wm_refresh_frames(void); /* 18R: tema değişiminde çerçeveleri tazele */
int wm_get_rect(int id, int* x, int* y, int* w, int* h); /* 18M: pencere geometrisi */
/* 18M: input yönlendirme */
int wm_from_comp(int comp_id);  /* comp yüzeyinden wm id (-1 yok) */
int wm_hit_test(int x, int y);  /* imleç altındaki en üst pencere (-1 masaüstü) */
void wm_drag_end(void);         /* sürüklemeyi bitir */
int wm_dragging(void);          /* sürüklenen id (-1 yok) */
/* Kısıt-3 (vsh windows komutu): pencere listesi + başlık */
int wm_list_ids(int* out, int max);
const char* wm_get_title(int id);

#endif
