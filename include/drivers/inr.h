#ifndef CHAROS_DRIVERS_INR_H
#define CHAROS_DRIVERS_INR_H

#include <stdint.h>

/* 18M: Input yönlendirme — hub olaylarını imleç altındaki/odaklı pencereye
 * dağıtır. Fare: hit-test + click-to-focus + başlık çubuğu sürükleme.
 * Klavye: odaklı pencereye (terminal -> term_input/PTY). */

int  inr_init(void);            /* imleç + masaüstü yerleşimi. 0 hazır. */
void inr_start_task(void);      /* task sistemi kurulduktan sonra hub görevi */
int  inr_poll(void);            /* bekleyen hub olaylarını dağıt. Dönüş: sayı */
int  inr_selftest(void);        /* hit/focus/drag/keys/cursor doğrula. 0 PASS. */

/* Sentetik + sürücü beslemesi (gerçek fare sürücüsü de bunları kullanır) */
void inr_mouse_move(int dx, int dy);
void inr_mouse_button(int buttons);      /* bit0: sol tuş */
int  inr_key_event(uint8_t scancode, int pressed); /* 1 yönlendirildi */

/* Durum sorguları */
int  inr_cursor_pos(int* x, int* y);
int  inr_routed_keys(void);
int  inr_routed_clicks(void);
/* Kısıt-2: klavye IRQ'su bu 1 ise tampona yazmaz; tuş hub üzerinden
 * odaklı terminale gider (çift yankı önlenir). IRQ bağlamında güvenli. */
int  inr_claims_keyboard(void);

#endif
