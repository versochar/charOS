#ifndef CHAROS_DRIVERS_USBDESC_H
#define CHAROS_DRIVERS_USBDESC_H

/* 33.2: USB yapılandırma tanımlayıcı yürüyücü (saf mantık).
 * Düşman cihazlara karşı: L=0 sonsuz döngü yapmaz, taşma sınır kontrollü.
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

/* cfg[0..total): HID boot klavye/fare kesme-IN uç arar.
 * Bulunursa 0 + çıktıları doldurur, yoksa -1. */
int usbdesc_find_hid_interrupt(const uint8_t* cfg, uint32_t total,
                               int8_t* out_iface, int* out_proto,
                               uint8_t* out_ep, uint32_t* out_mps,
                               uint8_t* out_interval);
int usbdesc_selftest(void); /* 0 ok */

#endif
