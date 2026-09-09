#ifndef CHAROS_MEMORY_PMM_H
#define CHAROS_MEMORY_PMM_H

#include <stdint.h>

#define PMM_MAX_MEMORY   (256 * 1024 * 1024)  // 256 MB
#define PMM_PAGE_SIZE    4096
#define PMM_MAX_FRAMES   (PMM_MAX_MEMORY / PMM_PAGE_SIZE) // 65536
#define PMM_BITMAP_SIZE  (PMM_MAX_FRAMES / 8) // 8192 byte

/* Multiboot info (Multiboot1) */
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} __attribute__((packed));

struct multiboot_mmap_entry {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed));

void pmm_init(uint32_t mboot_ptr);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t phys);
void pmm_reserve(uint32_t base, uint32_t length); /* 14G: havuzdan düş */
void pmm_inc_ref(uint32_t phys);
void pmm_dec_ref(uint32_t phys);
int pmm_get_ref(uint32_t phys);
uint32_t pmm_get_free_count(void);
uint32_t pmm_get_total_count(void);
void pmm_test(void);

#endif