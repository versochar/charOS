#ifndef CHAROS_DRIVERS_FB_H
#define CHAROS_DRIVERS_FB_H

#include <stdint.h>

/* 18B: Framebuffer v2 - VESA LFB (GRUB multiboot video), çift buffer,
 * page flip (memcpy), vblank senkronu. */

int fb_init(uint32_t mb_info);   /* 0 aktif, -1 fb bilgisi yok (SKIP) */
int fb_active(void);
void fb_get_mode(uint32_t* w, uint32_t* h, uint32_t* bpp);
/* 18C: compositor/gfx için ham erişim */
uint8_t* fb_back_buffer(void);
uint32_t fb_pitch_bytes(void);
uint8_t fb_bytes_per_pixel(void);

void fb_clear(uint32_t color);
void fb_putpixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_getpixel_front(uint32_t x, uint32_t y);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_flip(void);              /* kirli bölgeyi öne taşı (kısmi flip) */
void fb_damage(uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1);
void fb_damage_all(void);        /* tüm ekranı kirlet */
int fb_vblank_wait(void);        /* 1 senkron, 0 timeout */

int fb_selftest(void);           /* desen + flip + geriokuma. 0 PASS. */

#endif
