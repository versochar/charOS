#include <memory/paging.h>
#include <memory/pmm.h>
#include <core/isr.h>
#include <core/apic.h>
#include <process/task.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* Page Directory ve ilk Page Table - 4KB hizalı, identity 0-4MB */
__attribute__((aligned(4096))) uint32_t page_directory[1024];
__attribute__((aligned(4096))) uint32_t first_page_table[1024];
__attribute__((aligned(4096))) uint32_t kernel_page_table[1024]; // 4MB-8MB için (ileride kernel heap)

static inline void load_cr3(uint32_t phys) {
    asm volatile("mov %0, %%cr3" : : "r"(phys));
}
static inline void enable_paging(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG bit
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}
static inline uint32_t read_cr2(void) {
    uint32_t cr2; asm volatile("mov %%cr2, %0" : "=r"(cr2)); return cr2;
}
static inline uint32_t read_cr3(void) {
    uint32_t cr3; asm volatile("mov %%cr3, %0" : "=r"(cr3)); return cr3;
}

void paging_invalidate(uint32_t virt) {
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

void paging_map(uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;

    uint32_t table_phys;
    if (page_directory[dir_idx] & PAGE_PRESENT) {
        table_phys = page_directory[dir_idx] & 0xFFFFF000;
    } else {
        // Yeni bir page table (frame) ayır ve directory'ye bağla
        table_phys = pmm_alloc_frame();
        if (!table_phys) {
            vga_puts("paging_map: out of frames\n");
            for(;;) asm volatile("cli; hlt");
        }
        // table_phys 8MB+ olabilir (PMM reserve sonrası); identity varsayımı
        // geçersiz. Scratch pencere üzerinden sıfırla.
        memset(paging_scratch_phys(table_phys), 0, 4096);
        page_directory[dir_idx] = table_phys | PAGE_PRESENT | PAGE_RW |
                                  (flags & PAGE_USER);
        tlb_shootdown(virt & 0xFFC00000);
    }

    // Mevcut table da 8MB+ olabilir; PTE yazımı için scratch üzerinden eriş.
    uint32_t* table = (uint32_t*)paging_scratch_phys(table_phys);
    table[tbl_idx] = (phys & 0xFFFFF000) | (flags & 0xFFF) | PAGE_PRESENT;
    // 11C: PTE değişti -> diğer CPU'ların TLB'sini de geçersiz kıl (shootdown IPI)
    tlb_shootdown(virt);
}

void paging_unmap(uint32_t virt) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;
    if (!(page_directory[dir_idx] & PAGE_PRESENT)) return;
    uint32_t* table = (uint32_t*)(page_directory[dir_idx] & 0xFFFFF000);
    table[tbl_idx] = 0;
    // 11C: PTE kaldırıldı -> diğer CPU'ların TLB'sini de geçersiz kıl
    tlb_shootdown(virt);
}

/* ============================================================
 * 12A: Per-task address space - recursive mapping üzerinden
 * ============================================================ */

/* Mevcut (yüklü) page directory ve seçili dizin girişinin table'ı.
 * Recursive mapping her space'te kuruldu (dizin girişi 1023). */
static uint32_t* cur_pd(void) {
    return (uint32_t*)PAGE_DIR_SELF;
}
static uint32_t* cur_table(uint32_t dir_idx) {
    return (uint32_t*)(PAGE_RECURSIVE_BASE + dir_idx * 4096);
}

uint32_t paging_get_phys_current(uint32_t virt) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;
    uint32_t* pd = cur_pd();
    if (!(pd[dir_idx] & PAGE_PRESENT)) return 0;
    uint32_t* table = cur_table(dir_idx);
    if (!(table[tbl_idx] & PAGE_PRESENT)) return 0;
    return (table[tbl_idx] & 0xFFFFF000) | (virt & 0xFFF);
}
uint32_t paging_get_pte_flags(uint32_t virt) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;
    uint32_t* pd = cur_pd();
    if (!(pd[dir_idx] & PAGE_PRESENT)) return 0;
    uint32_t* table = cur_table(dir_idx);
    return table[tbl_idx] & 0xFFF;
}

/* Mevcut space'e (recursive) sor - 11C bounce-buffer doğrulaması dahil
 * tüm call-sitelere uyumlu. */
uint32_t paging_get_phys(uint32_t virt) {
    return paging_get_phys_current(virt);
}

/* 12A: Fiziksel sayfayı mevcut space'te SCRATCH_VADDR'e map eder. 4KB'lık
 * tek seferlik erişim; ardışık çağrılar pencereyi ezer, pointer saklanmaz. */
void* paging_scratch_phys(uint32_t phys) {
    uint32_t dir_idx = SCRATCH_VADDR >> 22;
    uint32_t tbl_idx = (SCRATCH_VADDR >> 12) & 0x3FF;
    uint32_t* pd = cur_pd();

    if (!(pd[dir_idx] & PAGE_PRESENT)) {
        uint32_t f = pmm_alloc_frame();
        if (!f) return 0;
        pd[dir_idx] = f | PAGE_PRESENT | PAGE_RW;
        memset(cur_table(dir_idx), 0, 4096); /* recursive görünüm üzerinden sıfırla */
    }
    uint32_t* table = cur_table(dir_idx);
    table[tbl_idx] = (phys & 0xFFFFF000) | PAGE_PRESENT | PAGE_RW;
    paging_invalidate(SCRATCH_VADDR);
    return (void*)SCRATCH_VADDR;
}

/* 12A: Yeni (boş, kernel map'li) address space. Kernel map'leri (identity
 * 0-8MB, LAPIC MMIO 0xFEE00000, recursive 1023) yeni space'e kopyalanır.
 * Dönen değer CR3 olarak yazılır. */
uint32_t paging_create_space(void) {
    uint32_t pd_phys = pmm_alloc_frame();
    if (!pd_phys) return 0;

    uint32_t* pd = (uint32_t*)paging_scratch_phys(pd_phys);
    if (!pd) { pmm_free_frame(pd_phys); return 0; }
    memset(pd, 0, 4096);

    /* Kernel tablolarını mevcut (yüklü) space'ten paylaş: tüm supervisor
     * (PAGE_USER = 0) dizin girişleri yeni space'e kopyalanır. Identity
     * 0-8MB (dizin 0,1) ve LAPIC MMIO (0xFEE00000 -> dizin 1019) gibi kernel
     * haritaları user bölgeleriyle örtüşmez -> güvenli paylaşım.
     * Scratch penceresi (992) hariç - her space kendi scratch tablosunu
     * lazily oluştursun (paylaşılan tablo karışmasın). */
    uint32_t* cur = cur_pd();
    uint32_t scratch_dir = SCRATCH_VADDR >> 22;
    for (int i = 0; i < 1023; i++) {
        if (i == (int)scratch_dir) continue;
        if (cur[i] & PAGE_PRESENT && !(cur[i] & PAGE_USER))
            pd[i] = cur[i];
    }
    pd[1023] = pd_phys | PAGE_PRESENT | PAGE_RW; /* recursive self-map */

    vga_puts("[12A] Address space created CR3=0x"); vga_puthex(pd_phys); vga_puts("\n");
    serial_puts("[12A] AS create cr3=0x"); serial_puthex(pd_phys); serial_puts("\n");
    return pd_phys;
}

/* 12A: CR3 yükle + TLB temizle (context switch) */
void paging_switch_space(uint32_t cr3) {
    load_cr3(cr3);
}

/* 12A: Belirli bir address space'e map ekle.
 *  - cr3 == mevcut ise recursive mapping ile doğrudan yaz.
 *  - aksi halde hedef pd/table scratch penceresinden düzenlenir.
 * TLB invalidation yalnızca mevcut space için gerekir (yabancı space
 * yüklü değilse orada stale TLB yoktur). */
void paging_map_in_space(uint32_t cr3, uint32_t virt, uint32_t phys, uint32_t flags) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;
    uint32_t pte = (phys & 0xFFFFF000) | PAGE_PRESENT | (flags & 0xFFF);

    if (cr3 == read_cr3()) {
        uint32_t* pd = cur_pd();
        uint32_t* table = cur_table(dir_idx);
        if (!(pd[dir_idx] & PAGE_PRESENT)) {
            uint32_t f = pmm_alloc_frame();
            if (!f) return;
            pd[dir_idx] = f | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
            memset(table, 0, 4096);
        }
        table[tbl_idx] = pte;
        paging_invalidate(virt);
        return;
    }

    /* Yabancı address space - scratch penceresi kullan */
    uint32_t* pd = (uint32_t*)paging_scratch_phys(cr3);
    if (!pd) return;
    uint32_t drent = pd[dir_idx];
    uint32_t table_phys;
    if (!(drent & PAGE_PRESENT)) {
        table_phys = pmm_alloc_frame();
        if (!table_phys) return;
        void* tv = paging_scratch_phys(table_phys);
        if (!tv) return;
        memset(tv, 0, 4096);
        pd = (uint32_t*)paging_scratch_phys(cr3);
        if (!pd) return;
        pd[dir_idx] = table_phys | PAGE_PRESENT | PAGE_RW | (flags & PAGE_USER);
    } else {
        table_phys = drent & 0xFFFFF000;
    }
    uint32_t* table = (uint32_t*)paging_scratch_phys(table_phys);
    if (!table) return;
    table[tbl_idx] = pte;
}

void paging_unmap_in_space(uint32_t cr3, uint32_t virt) {
    uint32_t dir_idx = virt >> 22;
    uint32_t tbl_idx = (virt >> 12) & 0x3FF;
    if (cr3 == read_cr3()) {
        uint32_t* pd = cur_pd();
        if (!(pd[dir_idx] & PAGE_PRESENT)) return;
        uint32_t* table = cur_table(dir_idx);
        table[tbl_idx] = 0;
        paging_invalidate(virt);
        return;
    }
    uint32_t* pd = (uint32_t*)paging_scratch_phys(cr3);
    if (!pd) return;
    uint32_t drent = pd[dir_idx];
    if (!(drent & PAGE_PRESENT)) return;
    uint32_t table_phys = drent & 0xFFFFF000;
    uint32_t* table = (uint32_t*)paging_scratch_phys(table_phys);
    if (!table) return;
    table[tbl_idx] = 0;
}

void page_fault_handler(struct registers* regs) {
    uint32_t faulting_address = read_cr2();
    int present   = !(regs->err_code & 0x1);
    int rw        = regs->err_code & 0x2;
    int us        = regs->err_code & 0x4;
    int reserved  = regs->err_code & 0x8;
    int id        = regs->err_code & 0x10;

    // 13B/C: CoW ve demand paging (VMA ile)
    // 13C: global current_task SMP'de fault eden task olmayabilir; per-CPU kullan
    extern struct task* current_task;
    extern struct task* current_per_cpu[];
    extern int cpu_id_get(void);
    struct task* fault_task = 0;
    {
        int _cpu = cpu_id_get();
        fault_task = current_per_cpu[_cpu] ? current_per_cpu[_cpu] : current_task;
    }
    if (us) {
        uint32_t va = faulting_address & ~0xFFF;
        uint32_t pte_flags = paging_get_pte_flags(va);
        int is_cow = (pte_flags & PAGE_COW) && (pte_flags & PAGE_PRESENT);
        // 13C: non-present fault (`present`==1, err bit0=0) ve CoW değilse
        // -> mmap demand / stack-code demand dene.
        // (Önceki `!present` koşulu ölü daldı; NP fault'lar hiç buraya giremiyordu.)
        if (present && !is_cow) {
            extern int mmap_handle_fault(uint32_t va, int write);
            if (fault_task && mmap_handle_fault(va, rw)) {
                serial_puts("[13C] mmap demand va=0x"); serial_puthex(va); serial_puts("\n");
                return;
            }
            if (fault_task && fault_task->cr3) {
                int in_stack = (va >= 0x1400000 && va < 0x1404000);
                int in_code = 0;
                for (int r = 0; r < fault_task->user_region_count; r++) {
                    uint32_t b = fault_task->user_regions[r].base;
                    uint32_t pages = fault_task->user_regions[r].pages;
                    if (va >= b && va < b + pages*0x1000) { in_code = 1; break; }
                }
                if (in_stack || in_code) {
                    uint32_t frame = pmm_alloc_frame();
                    if (frame) {
                        void* dst = paging_scratch_phys(frame);
                        memset(dst, 0, 0x1000);
                        paging_map_in_space(fault_task->cr3, va, frame, PAGE_USER | PAGE_RW);
                        serial_puts("[13B] demand alloc va=0x"); serial_puthex(va); serial_puts(" frame=0x"); serial_puthex(frame); serial_puts("\n");
                        return;
                    }
                }
            }
        }
        if (is_cow && rw) {
            // CoW: write fault on RO+COW page
            uint32_t old_phys = paging_get_phys(va);
            uint32_t old_frame = old_phys & ~0xFFF;
            uint32_t flags = pte_flags;
            if (pmm_get_ref(old_frame) > 1) {
                uint32_t new_frame = pmm_alloc_frame();
                if (!new_frame) {
                    serial_puts("[13B] CoW OOM\n");
                    goto kill;
                }
                void* src = paging_scratch_phys(old_frame);
                // src/dst aynı scratch penceresi: önce src'yi oku, SONRA dst'yi map et
                static uint8_t cow_tmp[4096];
                memcpy(cow_tmp, src, 4096);
                void* dst = paging_scratch_phys(new_frame);
                memcpy(dst, cow_tmp, 4096);
                pmm_dec_ref(old_frame);
                uint32_t new_flags = (flags & ~PAGE_COW) | PAGE_RW;
                uint32_t cr3 = fault_task ? fault_task->cr3 : read_cr3();
                paging_map_in_space(cr3, va, new_frame, new_flags);
                serial_puts("[13B] CoW va=0x"); serial_puthex(va); serial_puts(" old="); serial_puthex(old_frame); serial_puts(" new="); serial_puthex(new_frame); serial_puts("\n");
                return;
            } else {
                // refcount 1, sadece RW yap
                uint32_t new_flags = (flags & ~PAGE_COW) | PAGE_RW;
                uint32_t cr3 = fault_task ? fault_task->cr3 : read_cr3();
                uint32_t frame = old_frame;
                paging_map_in_space(cr3, va, frame, new_flags);
                serial_puts("[13B] CoW single va=0x"); serial_puthex(va); serial_puts("\n");
                return;
            }
        }
    }

    vga_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_puts("\n[PAGE FAULT] ");
    if (present) vga_puts("non-present page ");
    if (rw) vga_puts("write ");
    if (us) vga_puts("user-mode ");
    if (reserved) vga_puts("reserved-write ");
    if (id) vga_puts("instruction-fetch ");
    vga_puts("\nFault addr=0x"); vga_puthex(faulting_address);
    vga_puts(" EIP=0x"); vga_puthex(regs->eip);
    vga_puts(" Err=0x"); vga_puthex(regs->err_code);
    vga_puts("\n");

    serial_puts("\n[PAGE FAULT] fault=0x"); serial_puthex(faulting_address);
    serial_puts(" eip=0x"); serial_puthex(regs->eip);
    serial_puts(" err=0x"); serial_puthex(regs->err_code);
    serial_puts(" pid="); serial_puthex(fault_task ? (uint32_t)fault_task->pid : 0xFFFFFFFFu);
    if (present) serial_puts(" NP");
    if (rw) serial_puts(" W");
    if (us) serial_puts(" U");
    serial_puts("\n");

kill:
    /* 12A: User-mode fault -> sadece task'i öldür, sistemi kilitleme */
    extern void task_exit_and_switch(void);
    if (us && fault_task) {
        serial_puts("[12A] killing pid="); serial_puthex(fault_task->pid); serial_puts("\n");
        vga_puts("[12A] killing pid "); vga_putdec(fault_task->pid); vga_puts("\n");
        task_exit_and_switch();
    }

    for(;;) asm volatile("cli; hlt");
}

void paging_init(void) {
    // Page directory ve tabloları sıfırla
    memset(page_directory, 0, sizeof(page_directory));
    memset(first_page_table, 0, sizeof(first_page_table));
    memset(kernel_page_table, 0, sizeof(kernel_page_table));

    // Identity map 0-4MB (0x00000000 - 0x003FFFFF)
    for (int i = 0; i < 1024; i++) {
        first_page_table[i] = (i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
    }
    page_directory[0] = ((uint32_t)first_page_table) | PAGE_PRESENT | PAGE_RW;

    // 4MB-8MB için ikinci tablo (kernel heap ve ileride PMM için)
    // Şimdilik identity de map'le, ama ileride farklı map'lenebilir
    for (int i = 0; i < 1024; i++) {
        kernel_page_table[i] = ((4*1024*1024) + i * PAGE_SIZE) | PAGE_PRESENT | PAGE_RW;
    }
    page_directory[1] = ((uint32_t)kernel_page_table) | PAGE_PRESENT | PAGE_RW;

    // 12A: Recursive mapping - dizin 1023 kendi page directory'ni gösterir.
    // paging_get_phys_current / scratch pencere bu girişe dayanır.
    page_directory[1023] = ((uint32_t)&page_directory) | PAGE_PRESENT | PAGE_RW;

    // Page fault handler'ı kaydet (int 14)
    isr_register_handler(14, page_fault_handler);

    // CR3 yükle (fiziksel adres - identity olduğu için virtual == phys)
    load_cr3((uint32_t)page_directory);

    // Paging'i aç
    enable_paging();

    vga_puts("[3A] Paging enabled (identity 0-8MB, CR3=0x");
    vga_puthex((uint32_t)page_directory);
    vga_puts(")\n");
    serial_puts("[3A] Paging enabled CR3=0x"); serial_puthex((uint32_t)page_directory); serial_puts("\n");
}

void paging_enable(void) {
    load_cr3((uint32_t)page_directory);
    enable_paging();
}