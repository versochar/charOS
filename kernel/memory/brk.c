#include <memory/brk.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <process/task.h>
#include <core/spinlock.h>
#include <drivers/serial.h>
#include <string.h>

/* 14A: program break. Büyüyen sayfalar hemen alloc+map+zero yapılır
 * (is_user_buf_valid map edilmiş sayfa ister). Küçülmede sayfalar iade. */

static spinlock_t brk_lock = SPINLOCK_INIT;

uint32_t sys_brk(uint32_t new_brk) {
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur || !cur->cr3) return 0;
    uint32_t flags;
    spin_lock_irqsave(&brk_lock, &flags);
    if (cur->brk_base == 0) {
        cur->brk_base = BRK_BASE;
        cur->brk_cur = BRK_BASE;
    }
    uint32_t old = cur->brk_cur;
    if (new_brk == 0) { spin_unlock_irqrestore(&brk_lock, flags); return old; }
    if (new_brk < BRK_BASE || new_brk > BRK_MAX) {
        spin_unlock_irqrestore(&brk_lock, flags);
        return old;
    }
    if (new_brk > old) {
        uint32_t va = old & ~0xFFFu;
        uint32_t end = new_brk;
        while (va < end) {
            if (!paging_get_phys(va)) {
                uint32_t f = pmm_alloc_frame();
                if (!f) break;
                paging_map_in_space(cur->cr3, va, f,
                                    PAGE_PRESENT | PAGE_RW | PAGE_USER);
                task_add_region(cur, va, 1);
                memset((void*)va, 0, 0x1000);
            }
            va += 0x1000;
        }
        if (va < end) {
            /* OOM yarıda: kırılımı gerçekten map'lenen yere çek */
            cur->brk_cur = va;
            spin_unlock_irqrestore(&brk_lock, flags);
            return cur->brk_cur;
        }
        cur->brk_cur = new_brk;
    } else if (new_brk < old) {
        uint32_t va = (new_brk + 0xFFFu) & ~0xFFFu;
        uint32_t end = (old + 0xFFFu) & ~0xFFFu;
        while (va < end) {
            uint32_t phys = paging_get_phys(va);
            if (phys) {
                pmm_free_frame(phys & ~0xFFFu);
                paging_unmap_in_space(cur->cr3, va);
            }
            va += 0x1000;
        }
        cur->brk_cur = new_brk;
        /* user_regions girdileri bilerek durur (fork CoW map'siz sayfayı atlar) */
    }
    uint32_t ret = cur->brk_cur;
    spin_unlock_irqrestore(&brk_lock, flags);
    return ret;
}
