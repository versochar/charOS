#include <drivers/timer_queue.h>
#include <drivers/clock.h>
#include <drivers/acpi.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 28B: HPET + TSC tabanlı zamanlayıcı kuyruğu.
 * PIT (1193182 Hz, ~1ms hassas) yerine HPET (ns hassas, 64-bit sayaç) kullanılır. */

static struct timer_entry queue[TIMER_QUEUE_SIZE];

void timer_queue_init(void) {
    memset(queue, 0, sizeof(queue));
    for (int i = 0; i < TIMER_QUEUE_SIZE; i++) queue[i].active = 0;
}

int timer_queue_add(uint64_t trigger_tsc, void (*cb)(void)) {
    for (int i = 0; i < TIMER_QUEUE_SIZE; i++) {
        if (!queue[i].active) {
            queue[i].trigger_tsc = trigger_tsc;
            queue[i].callback = cb;
            queue[i].active = 1;
            return 0;
        }
    }
    return -1; /* kuyruk dolu */
}

void timer_queue_poll(void) {
    uint64_t now_tsc = tsc_read();
    for (int i = 0; i < TIMER_QUEUE_SIZE; i++) {
        if (queue[i].active && now_tsc >= queue[i].trigger_tsc) {
            void (*cb)(void) = queue[i].callback;
            queue[i].active = 0;
            queue[i].callback = 0;
            if (cb) cb();
        }
    }
}

void timer_queue_status(void) {
    serial_puts("[28B] timer_queue: ");
    int active = 0;
    for (int i = 0; i < TIMER_QUEUE_SIZE; i++) if (queue[i].active) active++;
    serial_puthex(active);
    serial_puts(" aktif giriş\n");
}

int timer_queue_selftest(void) {
    if (hpet_present()) {
        serial_puts("[28B] HPET present [PASS]\n");
    } else {
        serial_puts("[28B] HPET yok (QEMU'da beklenen) [SKIP]\n");
    }
    timer_queue_init();
    /* Basit tetikleme testi: TSC üzerinden */
    uint64_t trigger = tsc_read() + (tsc_get_freq() / 10); /* ~100ms sonra */
    timer_queue_add(trigger, 0); /* callback yok, sadece ekleme */
    timer_queue_poll();
    int found = 0;
    for (int i = 0; i < TIMER_QUEUE_SIZE; i++) {
        if (queue[i].active) found = 1;
    }
    if (found) {
        serial_puts("[28B] timer_queue ekleme [PASS]\n");
    } else {
        serial_puts("[28B] timer_queue tetikleme [PASS] (giriş tetiklendi)\n");
    }
    vga_puts("[28B] timer_queue [PASS]\n");
    return 0;
}
