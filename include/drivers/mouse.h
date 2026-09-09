#ifndef CHAROS_DRIVERS_MOUSE_H
#define CHAROS_DRIVERS_MOUSE_H

#include <stdint.h>
#include <core/isr.h>

/* PS/2 fare sürücüsü (i8042 AUX, IRQ12 -> int 44).
 * Göreli hareket + tuşlar input hub'a beslenir (input_mouse),
 * oradan inr imleci oynatır. Tüm init bekleyişleri timeout'ludur;
 * fare yoksa boot devam eder (present=0). */

void mouse_init(void);              /* cihazı aç, IRQ'yu kur (0 ok, -1 yok) */
void mouse_handler(struct registers* regs); /* IRQ12 */
int  mouse_present(void);           /* 1 algılandı */
/* IRQ + sentetik test beslemesi: ham PS/2 baytı paket çözümleyiciye */
void mouse_feed_byte(uint8_t b);

int  mouse_selftest(void);          /* çözümleyici + varlık. 0 PASS. */

#endif /* CHAROS_DRIVERS_MOUSE_H */
