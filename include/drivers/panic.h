#ifndef CHAROS_DRIVERS_PANIC_H
#define CHAROS_DRIVERS_PANIC_H

/* 30C: Hata toleransı - panic kurtarma, watchdog, temiz kapanış.
 * Skeleton: panic handler, watchdog timer, graceful shutdown. */

/* Panic handler (kernel hatası durumunda) */
void panic_handler(const char* msg, const char* file, int line);

/* Watchdog başlatma (basit: timer üzerinden) */
void watchdog_init(void);
void watchdog_feed(void);  /* watchdog'u besle (reset) */

/* Temiz kapanış (shutdown) */
void graceful_shutdown(void);

/* Watchdog self-test */
int watchdog_selftest(void);

#endif
