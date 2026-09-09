#ifndef CHAROS_MEMORY_MB2_H
#define CHAROS_MEMORY_MB2_H

#include <stdint.h>

#define MB1_BOOT_MAGIC 0x2BADB002
#define MB2_BOOT_MAGIC 0x36D76289

uint32_t mb2_translate(uint32_t mb2_addr);
uint32_t boot_info(uint32_t magic, uint32_t mboot_ptr);

/* 26A: UEFI ACPI RSDP kopyası (tag 14/15), yoksa 0 */
const void* mb2_rsdp(void);
uint32_t mb2_rsdp_len(void);

#endif