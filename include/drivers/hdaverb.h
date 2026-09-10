#ifndef CHAROS_DRIVERS_HDAVERB_H
#define CHAROS_DRIVERS_HDAVERB_H

/* 36.2: HDA anlık komut kodlayıcı (saf bit alanı mantığı).
 * Düzen: [31:28]=0, [27:20]=nid, [19:8]=verb, [7:0]=param.
 */
#include "stdint.h"

uint32_t hdaverb_build(uint32_t nid, uint32_t verb, uint32_t param);
uint32_t hdaverb_nid(uint32_t cmd);
uint32_t hdaverb_verb(uint32_t cmd);
uint32_t hdaverb_param(uint32_t cmd);

#endif
