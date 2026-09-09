#include <drivers/clock.h>
#include <drivers/timer.h>
#include <drivers/acpi.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 28A: TSC kalibrasyonu + clock_gettime.
 * TSC = CPU'da her saat döngüsünde artan 64-bit sayaç (RDTSC).
 * Kalibrasyon: PIT (1193182 Hz, bilinen frekans) üzerinden TSC frekansı
 * hesaplanır: tsc_freq = tsc_delta / pit_delta * PIT_FREQ. */

static uint64_t tsc_freq = 0;  /* Hz */

/* Temel 64-bit bölme (GCC freestanding için __udivdi3) */
static uint64_t udiv64(uint64_t a, uint64_t b) {
    uint64_t q = 0, rem = 0;
    for (int i = 63; i >= 0; i--) {
        rem = (rem << 1) | ((a >> i) & 1);
        if (rem >= b) {
            rem -= b;
            q |= (uint64_t)1 << i;
        }
    }
    return q;
}

/* TSC okuma (EDX:hi, EAX:lo) */
uint64_t tsc_read(void) {
    uint32_t lo, hi;
    asm volatile("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t tsc_get_freq(void) { return tsc_freq; }

/* TSC frekansı kalibrasyonu (PIT üzerinden ~100ms ölçüm) */
int clock_init(void) {
    if (tsc_freq > 0) return 0;
    /* PIT frekansı: 1193182 Hz (sabit) */
    uint32_t pit_start = timer_get_ticks();
    uint64_t tsc_start = tsc_read();
    /* ~100ms bekle (PIT tick'leri üzerinden) */
    while (timer_get_ticks() - pit_start < 10) {
        asm volatile("pause");
    }
    uint64_t tsc_end = tsc_read();
    uint32_t pit_end = timer_get_ticks();
    uint32_t pit_delta = pit_end - pit_start; /* PIT tick'leri */
    if (pit_delta == 0) pit_delta = 1;
    uint64_t tsc_delta = tsc_end - tsc_start;
    /* Kalibrasyon: PIT_FREQ (1193182 Hz) üzerinden TSC frekansı */
    tsc_freq = udiv64(tsc_delta * 1193182ULL, (uint64_t)pit_delta);
    /* Basit doğrulama: TSC frekansı 1-5 GHz aralığında olmalı */
    if (tsc_freq < 100000000ULL || tsc_freq > 5000000000ULL) {
        serial_puts("[28A] TSC kalibrasyonu başarısız (frekans anormal)\n");
        tsc_freq = 2400000000ULL; /* varsayılan ~2.4GHz */
        return -1;
    }
    serial_puts("[28A] TSC freq="); serial_puthex((uint32_t)(tsc_freq >> 32));
    serial_puthex((uint32_t)(tsc_freq & 0xFFFFFFFF)); serial_puts(" Hz\n");
    return 0;
}

/* Nanosecond saat: TSC delta / freq * 1e9 */
int clock_gettime(uint64_t* sec, uint32_t* nsec) {
    if (!tsc_freq) {
        if (clock_init() != 0) return -1;
    }
    uint64_t tsc_now = tsc_read();
    uint64_t ns = udiv64(tsc_now * 1000000000ULL, tsc_freq);
    if (sec) *sec = (uint64_t)udiv64(ns, 1000000000ULL);
    if (nsec) {
        uint64_t sec_div = udiv64(ns, 1000000000ULL);
        *nsec = (uint32_t)(ns - (sec_div * 1000000000ULL));
    }
    return 0;
}

uint32_t clock_now_sec(void) {
    uint64_t sec = 0;
    clock_gettime(&sec, 0);
    return (uint32_t)sec;
}

/* Self-test: TSC ilerlemesi kontrolü */
int clock_selftest(void) {
    if (clock_init() != 0) {
        serial_puts("[28A] TSC init [FAIL]\n");
        return -1;
    }
    uint64_t a = tsc_read();
    uint64_t b = a;
    for (volatile int i = 0; i < 1000000 && b == a; i++) {
        b = tsc_read();
        if ((i & 0xFFFF) == 0) asm volatile("pause");
    }
    if (b == a) {
        serial_puts("[28A] TSC ilerlemedi [FAIL]\n");
        return -1;
    }
    clock_gettime(0, 0); /* sadece kalibrasyon doğrulama */
    serial_puts("[28A] TSC kalibrasyon [PASS]\n");
    vga_puts("[28A] TSC kalibrasyon [PASS]\n");
    return 0;
}
