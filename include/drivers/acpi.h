#ifndef CHAROS_DRIVERS_ACPI_H
#define CHAROS_DRIVERS_ACPI_H

#include <stdint.h>

/* 14G: ACPI tabloları + güç yönetimi + HPET.
 * RSDP->RSDT->FADT/HPET zinciri doğrulanır. poweroff/reboot GERÇEK
 * geçiş yapar (testlerde ASLA çağrılmaz; yalnızca varlık/kayıt
 * doğrulanır). HPET sayacı monoton saat olarak okunur. */

int acpi_init(void);            /* tablo zinciri. 0 hazır. */
int acpi_have_fadt(void);
int acpi_have_hpet(void);
uint32_t acpi_pm1a_cnt(void);   /* PM1a CNT blok adresi (0=yok) */
uint16_t acpi_slp_typa(void);   /* S5 değeri (QEMU piix: 0x7<<10) */
void acpi_enable(void);         /* SCI enable (best-effort) */
void acpi_poweroff(void);       /* DÖNMEZ (sadece test dışı, gerçek geçiş) */
void acpi_reboot(void);         /* reset reg/kbd-pulse (testte çağrılmaz) */
void acpi_s3_sleep(void);       /* 28C: S3 uyku (best-effort skeleton) */

/* HPET (FADT/HPET tablosundan adreslenir, uncached map) */
int hpet_present(void);
uint32_t hpet_period_fs(void);  /* sayıcı periyodu (femtosaniye) */
uint64_t hpet_read(void);       /* 64-bit monoton sayaç */

int acpi_selftest(void);        /* RSDP/FACP/HPET/sayaç. 0 PASS. */

#endif
