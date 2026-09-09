#include <core/syslog.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14F: 4KB halka tampon. */

#define SYSLOG_SIZE 4096

static char syslog_buf[SYSLOG_SIZE];
static int syslog_head = 0;  /* yazım konumu */
static int syslog_count = 0; /* geçerli bayt */
static spinlock_t syslog_lock = SPINLOCK_INIT;
static int syslog_on = 0;

void syslog_init(void) {
    memset(syslog_buf, 0, sizeof(syslog_buf));
    syslog_head = 0;
    syslog_count = 0;
    syslog_lock.locked = 0;
    syslog_on = 1;
}

int syslog_puts(const char* s) {
    if (!syslog_on || !s) return -1;
    uint32_t flags;
    spin_lock_irqsave(&syslog_lock, &flags);
    int n = 0;
    while (s[n]) {
        syslog_buf[syslog_head] = s[n];
        syslog_head = (syslog_head + 1) % SYSLOG_SIZE;
        if (syslog_count < SYSLOG_SIZE) syslog_count++;
        n++;
    }
    spin_unlock_irqrestore(&syslog_lock, flags);
    return n;
}

int syslog_read(char* buf, int max) {
    if (!syslog_on || !buf || max <= 0) return -1;
    uint32_t flags;
    spin_lock_irqsave(&syslog_lock, &flags);
    int n = syslog_count;
    if (n > max - 1) n = max - 1;
    int start = (syslog_head - syslog_count + SYSLOG_SIZE * 2) % SYSLOG_SIZE;
    for (int i = 0; i < n; i++)
        buf[i] = syslog_buf[(start + i) % SYSLOG_SIZE];
    buf[n] = 0;
    syslog_head = 0;
    syslog_count = 0;
    spin_unlock_irqrestore(&syslog_lock, flags);
    return n;
}

int syslog_selftest(void) {
    int ok = 1;
    if (!syslog_on) syslog_init();
    {
        char tmp[64];
        syslog_read(tmp, sizeof(tmp)); /* öncekini temizle */
    }
    if (syslog_puts("SYSLOG-14F-OK\n") <= 0) ok = 0;
    {
        char buf[64];
        int n = syslog_read(buf, sizeof(buf));
        if (n <= 0) ok = 0;
        else {
            buf[n] = 0;
            const char* want = "SYSLOG-14F-OK\n";
            int i = 0;
            while (want[i] && buf[i] == want[i]) i++;
            if (want[i] != 0) ok = 0;
        }
        /* Okuma temizlemiş olmalı */
        if (syslog_read(buf, sizeof(buf)) != 0) ok = 0;
    }
    if (syslog_puts(0) != -1) ok = 0;
    if (ok) {
        serial_puts("[14F] syslog ring [PASS]\n");
        vga_puts("[14F] syslog ring [PASS]\n");
        return 0;
    }
    serial_puts("[14F] syslog [FAIL]\n");
    vga_puts("[14F] syslog [FAIL]\n");
    return -1;
}
