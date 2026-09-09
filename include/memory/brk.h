#ifndef CHAROS_MEMORY_BRK_H
#define CHAROS_MEMORY_BRK_H

#include <stdint.h>

/* 14A: per-process program break (user heap). Adresler demand değil,
 * büyürken hemen map'lenir (syscall tampon doğrulaması map ister). */

#define BRK_BASE 0x5000000u  /* 80MB: stack(~20MB) ve mmap(48-64MB) dışı */
#define BRK_MAX  0x6000000u  /* 96MB üst sınır */

uint32_t sys_brk(uint32_t new_brk); /* yeni kırılım / 0=sorgu; başarısızlıkta eski değer */

#endif
