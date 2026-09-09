#include <drivers/sysmon.h>
#include <drivers/timer.h>
#include <drivers/clock.h>
#include <drivers/thermal.h>
#include <drivers/blk.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* 29F: Sistem takibi skeleton */

void sysmon_init(void) {
    serial_puts("[29F] System monitor init\n");
}

void sysmon_update(void) {
    /* Temel: CPU yükü (timer tick sayısı), termal durum, disk */
}

void sysmon_draw(void) {
    /* Skeleton: durum çubuğu çizimi (gerçek GUI ilerki aşama) */
}

int sysmon_selftest(void) {
    sysmon_init();
    sysmon_update();
    sysmon_draw();
    serial_puts("[29F] System monitor [PASS]\n");
    vga_puts("[29F] System monitor [PASS]\n");
    return 0;
}
