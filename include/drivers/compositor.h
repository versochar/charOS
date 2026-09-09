#ifndef CHAROS_DRIVERS_COMPOSITOR_H
#define CHAROS_DRIVERS_COMPOSITOR_H

#include <stdint.h>

/* 18D: Compositor çekirdeği - yüzeyler, z-sırası, damage bölgeleri,
 * ekran birleştirme (arka buffer + flip). v1: opak blit, alfa yok. */

int comp_init(void);                       /* 0 hazır, -1 fb yok */
int comp_create(uint32_t w, uint32_t h);   /* yüzey id / -1 */
void comp_destroy(int id);
void comp_move(int id, int x, int y);      /* eski+yeni bölgeyi kirletir */
void comp_raise(int id);                   /* en üste al + tam kirlet */
void comp_fill(int id, uint32_t color);
void comp_fill_rect(int id, int x, int y, int w, int h, uint32_t color);
void comp_rect(int id, int x, int y, int w, int h, uint32_t color);
void comp_text(int id, int x, int y, const char* str, uint32_t color);
int comp_resize(int id, uint32_t w, uint32_t h); /* içeriği kırparak korur */
void comp_set_visible(int id, int v);
int comp_get_rect(int id, int* x, int* y, int* w, int* h);
void comp_damage(int id, int x, int y, int w, int h); /* yüzey-içi koordinat */
void comp_damage_all(int id);
void comp_set_bg(uint32_t color);
void comp_composite(void);                 /* kirli birleşimi çiz + flip */
struct gfx_surface* comp_surface(int id);  /* 18G: iç piksel erişimi */
int comp_selftest(void);                   /* z/damage/move doğrulama. 0 PASS. */

/* 18L: Animasyonlar (fade/slide). Frames adımında oynatılır.
 * comp_anim_step() her karede bir çağrılır; dönen: 1=hâlâ aktif, 0=bitti. */
void comp_fade_in(int id, int frames);
void comp_fade_out(int id, int frames);
void comp_slide_in(int id, int from_x, int from_y, int frames);
int comp_anim_step(void);                  /* tüm animasyonları 1 kare ilerlet */
int comp_anim_active(void);
int comp_anim_selftest(void);              /* 18L fade/slide doğrulama. 0 PASS. */

/* 18M: yazılım imleci (yüzey harcamaz, composite sonunda en üste çizilir) */
void comp_cursor_show(int x, int y);
void comp_cursor_hide(void);
void comp_cursor_move(int x, int y);
int comp_cursor_pos(int* x, int* y);       /* 1 görünür, 0 gizli */
/* 18M: alttan üste z-sıralı comp id listesi. Dönüş: yazılan sayı. */
int comp_zlist(int* out, int max);
/* 18Q: GUI katmanı (metin VT aktifken 0). Dönüşte full_repaint çağır. */
void comp_set_gui_active(int v);
int comp_gui_active(void);
void comp_full_repaint(void);
/* 18T: vsync (flip öncesi vblank bekle) + kare/skip sayaçları */
void comp_set_vsync(int v);
uint32_t comp_frame_count(void);
uint32_t comp_skip_count(void);

#endif
