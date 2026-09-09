#ifndef CHAROS_DRIVERS_USBHID_H
#define CHAROS_DRIVERS_USBHID_H

#include <stdint.h>

/* 26C: USB HID boot-protokol klavye/fare (xHCI üzerinden).
 * Raporlar input hub'a (klavye: keyboard_scancode ile PS/2 eşdeğeri,
 * fare: input_mouse) beslenir. */

int usbhid_init(void);      /* 0 = ≥1 cihaz, -1 = yok */
int usbhid_present(void);
int usbhid_selftest(void);  /* 0 PASS */
/* Kesme raporlarını tara + bekleyen TD'leri yeniden kur */
void usbhid_poll(void);
void usbhid_timer_poll(void); /* 100Hz timer kancası */

#endif
