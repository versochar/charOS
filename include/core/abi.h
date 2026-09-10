#ifndef CHAROS_CORE_ABI_H
#define CHAROS_CORE_ABI_H

/* 30.2: ABI kararlılık bekçisi.
 * - Dondurulmuş numaralar: tarihsel nr->ad eşleşmesi değişemez.
 * - Kapsama: kayıtlı her syscall'ın belgesi olmalı (syscall.c tarafı).
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

int abi_frozen_check(void);   /* 0 ok: dondurulmuş eşleşmeler korunur */
int abi_selftest(void);       /* 0 ok */
int syscall_abi_check(void);  /* 0 ok: tanımı syscall.c içinde */

#endif
