#ifndef CHAROS_MEMORY_PAGING_H
#define CHAROS_MEMORY_PAGING_H

#include <stdint.h>
#include <core/isr.h>

#define PAGE_SIZE       4096
#define PAGE_ENTRIES    1024
#define PAGE_DIR_ENTRIES 1024

/* Page table entry flags */
#define PAGE_PRESENT    0x001
#define PAGE_RW         0x002
#define PAGE_USER       0x004
#define PAGE_PWT        0x008
#define PAGE_PCD        0x010
#define PAGE_ACCESSED   0x020
#define PAGE_DIRTY      0x040
#define PAGE_PAT        0x080
#define PAGE_GLOBAL     0x100
#define PAGE_COW        0x200  // 13B: Copy-on-Write (avaliable bit 9)

/* 12A: Recursive page-directory mapping.
 * Her address space'in son dizin girişi (1023) kendi page directory'sini gösterir:
 *   - PAGE_DIR_SELF   (0xFFFFF000) : mevcut page directory'nin kendisi
 *   - PAGE_RECURSIVE  (0xFFC00000 + dir*4096) : ilgili dizin girişinin page table'ı
 */
#define PAGE_RECURSIVE_BASE  0xFFC00000
#define PAGE_DIR_SELF        0xFFFFF000

/* 12A: Kernel scratch penceresi - yabancı (yüklü olmayan) fiziksel sayfaları
 * mevcut address space'e geçici map etmek için. User sanal adresleriyle ve
 * LAPIC MMIO (0xFEE00000) ile çakışmaz. */
#define SCRATCH_VADDR        0xF8000000

/* 12A: Per-task address space API */
#define TASK_MAX_REGIONS     8

/* Bir user task'ının sahip olduğu sanal bölge (code/stack) */
struct task_region {
    uint32_t base;   /* sanal başlangıç adresi */
    uint32_t pages;  /* page sayısı */
};

void paging_init(void);
void paging_enable(void);
void paging_map(uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap(uint32_t virt);
uint32_t paging_get_phys(uint32_t virt);
void paging_invalidate(uint32_t virt);
void page_fault_handler(struct registers* regs);

/* 12A: Yeni (boş, kernel map'li) bir address space oluşturur. Dönen değer
 * CR3'e yazılacak fiziksel page directory adresidir, 0 ise OOM. */
uint32_t paging_create_space(void);
/* 12A: CR3 yükleyip TLB'yi temizler (context switch'te çağrılır) */
void paging_switch_space(uint32_t cr3);
/* 12A: Belirli bir address space'e sanal->fiziksel map ekler. cr3 == mevcut
 * ise recursive mapping, aksi halde scratch pencere kullanılır. */
void paging_map_in_space(uint32_t cr3, uint32_t virt, uint32_t phys, uint32_t flags);
void paging_unmap_in_space(uint32_t cr3, uint32_t virt);
/* 12A: Mevcut (yüklü) address space'in fiziksel adresini sorgular */
uint32_t paging_get_phys_current(uint32_t virt);
uint32_t paging_get_pte_flags(uint32_t virt);
/* 12A: Verilen fiziksel sayfayı mevcut space'te SCRATCH_VADDR'e map edip
 * pointer döndürür. 4KB'lık tek seferlik erişim içindir; ardışık iki çağrı
 * aynı pencereyi kullanır. */
void* paging_scratch_phys(uint32_t phys);

extern uint32_t page_directory[1024];
extern uint32_t first_page_table[1024];

#endif