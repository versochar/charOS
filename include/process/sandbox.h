#ifndef CHAROS_PROCESS_SANDBOX_H
#define CHAROS_PROCESS_SANDBOX_H

/* 23.2: Görev başına syscall filtresi (seccomp64 stub'undan farklı:
 * global değil, task'a bağlı; syscall_handler içinde zorlanır).
 */
#include "stdint.h"

#define SB_MASK_WORDS 8   /* 8x32 = 256 syscall */
#define SB_MAX_NR 256

/* Saf bitmask mantığı: task yapısına dokunmaz, host'ta da derlenir. */
void sb_allow_all(uint32_t* mask);
void sb_lockdown(uint32_t* mask);              /* hepsini yasakla */
int  sb_allow(uint32_t* mask, uint32_t nr);    /* 0 ok / -1 hata */
int  sb_deny(uint32_t* mask, uint32_t nr);     /* 0 ok / -1 hata */
int  sb_check(const uint32_t* mask, uint32_t nr); /* 1=serbest, 0=engelli */

#endif
