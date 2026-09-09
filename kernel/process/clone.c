#include <process/task.h>
#include <process/fork.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 13F: Thread'in iret frame'i USER stack'e RING 0'da yazılır (enter_user_mode_fork).
 * Bu yazım COW/read-only sayfaya denk gelirse us=0 (kernel) fault olduğundan
 * page_fault_handler ele almaz -> #DF -> reset. Çözüm: thread'i çalıştırmadan
 * önce kullanıcı stack'in üst (frame) sayfasını present+rw yap. */
static void clone_pmap_thread_stack(struct task* t) {
    extern uint32_t paging_get_pte_flags(uint32_t virt);
    extern void* paging_scratch_phys(uint32_t phys);
    extern void paging_invalidate(uint32_t virt);
    extern int pmm_get_ref(uint32_t frame);
    extern void pmm_dec_ref(uint32_t frame);
    extern void paging_map_in_space(uint32_t cr3, uint32_t virt, uint32_t phys, uint32_t flags);
    extern uint32_t paging_get_phys(uint32_t virt);

    uint32_t top = t->user_stack_addr;
    /* iret frame esp'nin ALTINDADIR; thread de kendi stack'ini aşağı iter.
     * İki sayfayı güvenle map et: top sayfası + hemen altındaki. */
    uint32_t first_page = ((top - 4096) & ~0xFFF) ? ((top - 4096) & ~0xFFF) : (top & ~0xFFF);

    for (uint32_t cur_va = (top & ~0xFFF); ; cur_va -= 0x1000) {
        uint32_t flags = paging_get_pte_flags(cur_va);
        uint32_t phys = paging_get_phys(cur_va);

        if ((flags & PAGE_PRESENT) && (flags & PAGE_COW)) {
            uint32_t old_frame = phys & ~0xFFF;
            uint32_t new_frame = pmm_alloc_frame();
            if (new_frame) {
                void* src = paging_scratch_phys(old_frame);
                void* dst = paging_scratch_phys(new_frame);
                if (src && dst) memcpy(dst, src, 4096);
                if (pmm_get_ref(old_frame) > 1) pmm_dec_ref(old_frame);
                paging_map_in_space(t->cr3, cur_va, new_frame, PAGE_USER | PAGE_RW);
                serial_puts("[CLONE-P] cow va=0x"); serial_puthex(cur_va);
                serial_puts(" old=0x"); serial_puthex(old_frame);
                serial_puts(" new=0x"); serial_puthex(new_frame); serial_puts("\n");
            }
        } else if (!(flags & PAGE_PRESENT)) {
            uint32_t frame = pmm_alloc_frame();
            if (frame) {
                void* p = paging_scratch_phys(frame);
                if (p) memset(p, 0, 4096);
                paging_map_in_space(t->cr3, cur_va, frame, PAGE_USER | PAGE_RW);
                serial_puts("[CLONE-P] demand va=0x"); serial_puthex(cur_va);
                serial_puts(" frame=0x"); serial_puthex(frame); serial_puts("\n");
            }
        }
        if (cur_va == first_page) break;
    }
}

/* 13F: Gerçek clone/threads - aynı address space (cr3 paylaşımı), yeni
 * kernel stack, yeni user stack pointer, aynı fd table referansı.
 * Thread, parent'ın kod sayfalarını paylaşır; veri de aynı AS içinde inode'dur. */

struct task* task_clone_thread(uint32_t entry, uint32_t user_stack) {
    // 13F: global current_task SMP'de yanlış task olabilir; per-CPU kullan
    int cpu = cpu_id_get();
    struct task* parent = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!parent || !parent->cr3) return 0;

    struct task* t = task_prepare("thread");
    if (!t) return 0;
    t->cr3 = parent->cr3;          // aynı adres alanı
    t->is_thread = 1;
    t->pgid = parent->pgid;
    t->parent_pid = parent->pid;
    t->uid = parent->uid; /* 14E: thread kimliği paylaşır */
    t->gid = parent->gid;
    t->brk_base = parent->brk_base; /* 14A: heap paylaşılır */
    t->brk_cur = parent->brk_cur;
    t->user_tls = 0; /* 15C: thread kendi TLS bloğunu kurar (pthread trambolini) */

    // FD tablosu referansı paylaş (kopya ama pipe referansları artırılır)
    fd_table_copy(t, parent);
    t->vmas = parent->vmas;        // VMA listesi: thread'ler paylaşır
    t->priority = parent->priority;
    t->ticks_left = parent->ticks_left;
    t->sleep_until = 0;

    // User state: entry noktası ve yeni user stack
    t->user_eip = entry;
    t->user_esp = user_stack;
    t->user_eflags = 0x202;
    t->user_cs = 0x1B;
    t->user_ss = 0x23;
    t->user_stack_addr = user_stack;

    // Kernel stack'i fork_child_finalize'a kur (iret user esp/eip ile, eax=0)
    extern void fork_child_finalize(void);
    extern void task_bootstrap(void);
    uint32_t* sp = (uint32_t*)t->stack_top;
    *--sp = (uint32_t)fork_child_finalize;
    *--sp = (uint32_t)task_bootstrap;
    *--sp = 0; *--sp = 0; *--sp = 0; *--sp = 0;
    t->esp = (uint32_t)sp;
    t->eip = (uint32_t)task_bootstrap;

    vga_puts("[13F] clone pid="); vga_putdec(t->pid);
    vga_puts(" entry=0x"); vga_puthex(entry);
    vga_puts(" esp=0x"); vga_puthex(user_stack); vga_puts("\n");
    serial_puts("[13F] clone pid="); serial_puthex(t->pid);
    serial_puts(" share_cr3=0x"); serial_puthex(parent->cr3); serial_puts("\n");

    // 13F: iret frame'inin yazılacağı stack sayfasını present+rw yap
    clone_pmap_thread_stack(t);
    return t;
}
