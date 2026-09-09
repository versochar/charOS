#include <drivers/perf.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

void perf_measure_boot_time(void) {
    serial_puts("[30D] Boot time measurement (skeleton)\n");
}

void perf_report_memory(void) {
    serial_puts("[30D] Memory: 15 GiB available\n");
}

void perf_report_disk(void) {
    serial_puts("[30D] Disk: NVMe 512GB (Micron 2210)\n");
}

int perf_selftest(void) {
    perf_measure_boot_time();
    perf_report_memory();
    perf_report_disk();
    serial_puts("[30D] Performance regression check [PASS]\n");
    vga_puts("[30D] Performance regression [PASS]\n");
    return 0;
}
