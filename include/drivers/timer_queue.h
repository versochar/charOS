#ifndef CHAROS_DRIVERS_TIMER_QUEUE_H
#define CHAROS_DRIVERS_TIMER_QUEUE_H

#include <stdint.h>

/* 28B: HPET tabanlı zamanlayıcı kuyruğu.
 * PIT yerine HPET (64-bit monoton sayaç, ns hassas) kullanılır. */

/* Zamanlayıcı girişi */
struct timer_entry {
    uint64_t trigger_tsc;   /* TSC değeri (tetikleme) */
    void (*callback)(void); /* çağrı */
    int active;             /* 1=aktif, 0=yok */
};

#define TIMER_QUEUE_SIZE 16

/* Kuyruk başlatma */
void timer_queue_init(void);

/* Yeni giriş ekle (trigger_tsc: TSC sayacı) */
int timer_queue_add(uint64_t trigger_tsc, void (*cb)(void));

/* Kuyruğu tara ve tetiklenenleri çağır */
void timer_queue_poll(void);

/* Kuyruk durumunu yazdır (tanılama) */
void timer_queue_status(void);

/* Self-test: HPET + kuyruk çalışması */
int timer_queue_selftest(void);

#endif
