#ifndef CHAROS_CORE_SYSLOG_H
#define CHAROS_CORE_SYSLOG_H

#include <stdint.h>

/* 14F: çekirdek halka tamponu (syslog). Yazan taşınca en eskiyi ezer;
 * okuma tamponu boşaltır. IRQ/task güvenli (spinlock). */

#define SYSLOG_READ_CLEAR 0  /* tamponu kullanıcıya kopyala + temizle */
#define SYSLOG_WRITE      1  /* kullanıcı metnini ekle */

void syslog_init(void);
int syslog_puts(const char* s);                    /* eklenen bayt */
int syslog_read(char* buf, int max);               /* kopyalanan bayt, temizler */
int syslog_selftest(void);                         /* yaz/oku döngüsü. 0 PASS. */

#endif
