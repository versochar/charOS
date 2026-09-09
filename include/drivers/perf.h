#ifndef CHAROS_DRIVERS_PERF_H
#define CHAROS_DRIVERS_PERF_H

/* 30D: Performans regresyonu (boot süresi, bellek, disk) */
void perf_measure_boot_time(void);
void perf_report_memory(void);
void perf_report_disk(void);
int perf_selftest(void);

#endif
