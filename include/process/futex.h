#ifndef CHAROS_PROCESS_FUTEX_H
#define CHAROS_PROCESS_FUTEX_H

#include <stdint.h>

/* 14D: Futex — kullanıcı adresi + adres alanı (cr3) anahtarlı bekleme
 * kuyrukları. Linux-stili bloklama, pthread-lite temeli. */

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_MAX  16  /* eşzamanlı bekleyen slotu */

int futex_init(void);
int futex_wait(uint32_t uaddr, uint32_t val); /* eşleşirse bloklanır, 0 uyandı */
int futex_wake(uint32_t uaddr, int n);        /* uyandırılan sayısı */
int futex_selftest(void);                     /* EAGAIN + boş uyandırma. 0 PASS. */

#endif
