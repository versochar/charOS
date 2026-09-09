#include <process/fork.h>
#include <process/task.h>
#include <process/pipe.h>
#include <memory/paging.h>
#include <memory/mmap.h>
#include <memory/pmm.h>
#include <memory/kheap.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>
#include <core/isr.h>
#include <core/spinlock.h>

/* 13B: CoW Fork - parent PTE'leri RO+COW, child aynı frame'leri paylaşır, refcount++.
 * Demand paging için stack'in en üst sayfası dışındakiler lazy (fault'ta alloc). */

struct task* fork_task(struct registers* regs) {
    if (!regs) return 0;
    struct task* parent = task_current();
    if (!parent) return 0;
    if (!parent->cr3) {
        vga_puts("[12B] fork: parent has no cr3 (kernel task) - refused\n");
        serial_puts("[12B] fork: no cr3\n");
        return 0;
    }

    /* Boş child task (kernel stack + PID + listeye ekli) */
    struct task* child = task_prepare("fork_child");
    if (!child) return 0;

    /* Yeni address space */
    uint32_t child_cr3 = paging_create_space();
    if (!child_cr3) {
        child->state = TASK_DEAD;
        return 0;
    }
    child->cr3 = child_cr3;

    // 13B: CoW - parent ve child aynı frame'i paylaşır, PTE'ler RO+COW
    extern uint32_t paging_get_pte_flags(uint32_t vaddr);
    for (int r = 0; r < parent->user_region_count; r++) {
        uint32_t base  = parent->user_regions[r].base;
        uint32_t pages = parent->user_regions[r].pages;
        task_add_region(child, base, pages);
        for (uint32_t i = 0; i < pages; i++) {
            uint32_t vaddr = base + i * 0x1000;
            uint32_t phys = paging_get_phys(vaddr);
            if (!phys) continue; // demand - henüz yoksa atla
            uint32_t frame = phys & ~0xFFF;
            uint32_t flags = paging_get_pte_flags(vaddr);
            uint32_t new_flags = (flags & ~PAGE_RW) | PAGE_COW | PAGE_USER | PAGE_PRESENT;
            // CoW için RW temizle, COW set et, PRESENT koru
            new_flags = (new_flags & ~0xFFF) | (new_flags & 0xFFF);
            // Aslında flags zaten PAGE_USER | PAGE_PRESENT içerir, sadece RW->COW
            new_flags = (flags | PAGE_COW) & ~PAGE_RW;
            new_flags |= PAGE_PRESENT | PAGE_USER;
            pmm_inc_ref(frame);
            uint32_t parent_cr3 = parent->cr3 ? parent->cr3 : (uint32_t)page_directory;
            paging_map_in_space(child_cr3, vaddr, frame, new_flags);
            paging_map_in_space(parent_cr3, vaddr, frame, new_flags);
            paging_invalidate(vaddr);
        }
    }

    fd_table_copy(child, parent);
    child->parent_pid = parent->pid;
    child->uid = parent->uid; /* 14E: kimlik miras */
    child->gid = parent->gid;
    child->brk_base = parent->brk_base; /* 14A: heap miras (sayfalar CoW) */
    child->brk_cur = parent->brk_cur;
    child->user_tls = parent->user_tls; /* 15C: TLS tabanı miras */
    // 13C: VMA listesini kopyala (shared/private için CoW)
    child->vmas = 0;
    struct vm_area* cur_vma = parent->vmas;
    struct vm_area** child_tail = &child->vmas;
    while (cur_vma) {
        struct vm_area* n = (struct vm_area*)kmalloc(sizeof(struct vm_area));
        if (!n) break;
        *n = *cur_vma;
        n->next = 0;
        // MAP_SHARED ise aynı VMA'yı paylaş, Private ise CoW (zaten PTE CoW)
        *child_tail = n;
        child_tail = &n->next;
        cur_vma = cur_vma->next;
    }
    child->priority = parent->priority;
    child->ticks_left = parent->ticks_left;

    /* User register state: int 0x80 trap frame'den */
    child->user_eip    = regs->eip;
    child->user_esp    = regs->useresp;
    /* H1 FIX: trap eflags'ı ham kopyalama! TF (single-step), DF, IOPL, NT
     * gibi sistem bitleri çocuğa sızarsa ilk user komutunda #DB (int 1,
     * loglarda efl=0x3FD2) atar. Durum bitlerini koru (CF/PF/AF/ZF/SF/OF),
     * TF/DF/IOPL/NT'yi temizle, IF'yi zorla aç. (exec/clone zaten 0x202.) */
    child->user_eflags = (regs->eflags & 0x8D7u) | 0x202u;
    child->user_cs     = regs->cs;
    child->user_ss     = regs->ss;
    child->user_stack_addr = regs->useresp;
    /* General regs for full restore (iret sonrası eax=0 hariç) */
    child->fork_edi = regs->edi;
    child->fork_esi = regs->esi;
    child->fork_ebp = regs->ebp;
    child->fork_ebx = regs->ebx;
    child->fork_edx = regs->edx;
    child->fork_ecx = regs->ecx;

    /* Child kernel stack: task_bootstrap -> fork_child_finalize -> iret.
     * task_prepare child->esp'i 0 verdi; burada bootstrap layout'u kur. */
    extern void task_bootstrap(void);
    uint32_t* sp = (uint32_t*)child->stack_top;
    *--sp = (uint32_t)fork_child_finalize;
    *--sp = (uint32_t)task_bootstrap;
    *--sp = 0; /* ebp */
    *--sp = 0; /* ebx */
    *--sp = 0; /* esi */
    *--sp = 0; /* edi */
    child->esp = (uint32_t)sp;
    child->ebp = 0;
    child->eip = (uint32_t)task_bootstrap;

    vga_puts("[12B] Fork created pid "); vga_putdec(child->pid);
    vga_puts(" from pid "); vga_putdec(parent->pid);
    vga_puts(" cr3=0x"); vga_puthex(child_cr3); vga_puts("\n");
    serial_puts("[12B] Fork pid="); serial_puthex(child->pid);
    serial_puts(" child_cr3=0x"); serial_puthex(child_cr3);
    serial_puts(" parent="); serial_puthex(parent->pid); serial_puts("\n");

    return child;
}
