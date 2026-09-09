#ifndef CHAROS_CORE_BOOT_FLOW_H
#define CHAROS_CORE_BOOT_FLOW_H

#include <stdint.h>

/* 30A: Açılış akışı - GRUB -> kernel -> init -> login -> masaüstü (otomatik).
 * Skeleton: init sırası doğrulama + otomatik masaüstü başlatma. */

/* Açılış sırası doğrulama */
int boot_flow_check(void);

/* Otomatik masaüstü başlatma (init sonrası) */
void boot_start_desktop(void);

/* Açılış süresi ölçümü (basit) */
uint32_t boot_duration_ms(void);

#endif
