#ifndef CHAROS_DRIVERS_APP_PLATFORM_H
#define CHAROS_DRIVERS_APP_PLATFORM_H

#include <stdint.h>

/* 29C: Uygulama platformu - pencere yöneticisi + TR klavye + fare hızı.
 * Gerçek pencere sistemi (kompozitör, pencere ağacı, odak) karmaşık;
 * skeleton: pencere yöneticisi başlatma + klavye hızı ayarı. */

/* Pencere yöneticisi başlatma */
int window_manager_init(void);

/* TR klavye düzeni ayarı (TR_Q klavye) */
void keyboard_tr_layout(void);

/* Fare hızı ayarı */
void mouse_speed_set(uint32_t speed);

/* Self-test: pencere başlatma + TR klavye + fare */
int app_platform_selftest(void);

#endif
