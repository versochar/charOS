#include <process/task.h>
#include <process/cap.h>
#include <process/sandbox.h>
#include <memory/kheap.h>
#include <memory/paging.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <string.h>
#include <core/gdt.h>
#include <fs/fd.h>
#include <fs/vfs.h>
#include <process/pipe.h>

struct task* current_task = 0;
struct task* current_per_cpu[MAX_CPU];
struct task* task_list = 0;
static struct task* idle_task = 0;
static int next_pid = 1;
spinlock_t task_lock = SPINLOCK_INIT;
static spinlock_t rq_lock[MAX_CPU];

/* Task switch assembly - saves current, restores next */
extern void task_switch(uint32_t* old_esp, uint32_t* new_esp);
extern void jump_to_user_mode(uint32_t entry, uint32_t stack);

/* Ring 3'e geçiş bootstrap'i - ilk kez schedule edildiğinde çağrılır */
static void enter_user_mode(void) {
    jump_to_user_mode(current_task->user_eip, current_task->user_esp);
    for(;;) asm volatile("hlt"); // dönmez
}

static void idle_func(void) {
    /* Boşta bekleyen görev: hlt ile CPU'yu bırakır, çıktı üretmez
     * (eski periyodik serial spam kaldırıldı — log gürültüsüydü). */
    for(;;) {
        asm volatile("sti; hlt");
    }
}

/* TSS stack güncelle */
extern void tss_set_stack(uint32_t ss0, uint32_t esp0);
extern uint32_t page_directory[1024];

static void fd_table_init(struct task* t);
void fd_table_copy(struct task* dst, struct task* src);

extern uint8_t stack_top; // boot.s'den
void task_init(void) {
    // Ana kernel task'ı için struct oluştur (mevcut stack'i kullan)
    struct task* main_task = (struct task*)kmalloc(sizeof(struct task));
    memset(main_task, 0, sizeof(struct task));
    fd_table_init(main_task);
    main_task->pid = next_pid++;
    main_task->pgid = main_task->pid;
    main_task->state = TASK_RUNNING;
    main_task->stack = 0; // ana stack boot.s'deki stack, kmalloc değil
    main_task->stack_top = (uint32_t)&stack_top;
    main_task->esp = 0;
    main_task->next = 0;
    current_task = main_task;
    task_list = main_task;

    // Idle task oluştur
    idle_task = task_create(idle_func, "idle");
    idle_task->priority = 0; // en düşük

    // 11B: Per-CPU scheduler init
    for (int i = 0; i < MAX_CPU; i++) {
        rq_lock[i].locked = 0; // SPINLOCK_INIT yerine
        current_per_cpu[i] = 0;
    }
    current_per_cpu[0] = main_task;

    vga_puts("[4A] Task init: main pid "); vga_putdec(main_task->pid);
    vga_puts(" idle pid "); vga_putdec(idle_task->pid); vga_puts("\n");
    serial_puts("[4A] Task init main="); serial_puthex(main_task->pid);
    serial_puts(" idle="); serial_puthex(idle_task->pid); serial_puts("\n");
}

struct task* task_current(void) {
    // 13F: global current_task SMP'de o CPU'da çalışan task olmayabilir.
    // Per-CPU kaydı öncelikli; yoksa global'e düş.
    int cpu = cpu_id_get();
    if (cpu >= 0 && cpu < MAX_CPU && current_per_cpu[cpu])
        return current_per_cpu[cpu];
    return current_task;
}

void task_set_user(uint32_t esp, uint32_t eip, uint32_t cs, uint32_t ss) {
    if (!current_task) return;
    current_task->user_esp = esp;
    current_task->user_eip = eip;
    current_task->user_cs = cs;
    current_task->user_ss = ss;
    current_task->user_eflags = 0x202; // IF=1
    current_task->user_stack_addr = esp;
}

void task_exit_and_switch(void) {
    task_exit();
}

void task_add_region(struct task* t, uint32_t base, uint32_t pages) {
    if (!t || t->user_region_count >= TASK_MAX_REGIONS) return;
    t->user_regions[t->user_region_count].base = base;
    t->user_regions[t->user_region_count].pages = pages;
    t->user_region_count++;
}

static void fd_table_init(struct task* t) {
    for (int i = 0; i < 16; i++) {
        t->fd_table[i].valid = 0;
        t->fd_table[i].type = FD_NONE;
        t->fd_table[i].pipe = 0;
    }
    // 12D: stdin/stdout/stderr her zaman ayrık (OPEN bunları vermez,
    // READ(0) klavye olarak çalışır). Console fd'ler kapatılamaz.
    for (int i = 0; i < 3; i++) {
        t->fd_table[i].valid = 1;
        t->fd_table[i].type = FD_CONSOLE;
        t->fd_table[i].inode_id = -1;
    }
    for (int i = 0; i < 32; i++) t->sig_handlers[i] = 0;
    t->sig_pending = 0;
    t->sig_mask = 0;
    t->vmas = 0;
    t->brk_base = 0; /* 14A: ilk brk çağrısında kurulur */
    t->brk_cur = 0;
    t->uid = 0; /* 14E: varsayılan root */
    t->gid = 0;
    t->caps = CAP_ALL; /* 22.4: root bootstrap tam yetki; fork yolu fork.c'de ezer */
    t->ngroups = 0;
    sb_allow_all(t->sb_mask); /* 23.4: sandbox varsayılanı serbest */
    t->sb_on = 0;
    t->priority = 2; // 13D: default normal (0 idle, 1 high, 2 normal, 3 low)
    t->ticks_left = 10;
    t->exit_status = 0;
    t->is_thread = 0;
    t->pgid = 0; /* 13E: pgid = pid, doldurulur */
}
void fd_table_copy(struct task* dst, struct task* src) {
    for (int i = 0; i < 16; i++) {
        dst->fd_table[i] = src->fd_table[i];
        if (dst->fd_table[i].valid) {
            if (dst->fd_table[i].type == FD_FILE) {
                int ino = dst->fd_table[i].inode_id;
                if (ino >= 0 && ino < FS_MAX_FILES) {
                    extern struct inode inode_table[FS_MAX_FILES];
                    extern int inode_used[FS_MAX_FILES];
                    if (inode_used[ino]) inode_table[ino].ref_count++;
                }
            } else if (dst->fd_table[i].type == FD_PIPE) {
                struct pipe* p = dst->fd_table[i].pipe;
                if (p) {
                    uint32_t flags;
                    spin_lock_irqsave(&p->lock, &flags);
                    if (dst->fd_table[i].pipe_write) p->writers++;
                    else p->readers++;
                    p->ref_count++;
                    spin_unlock_irqrestore(&p->lock, flags);
                }
            }
        }
    }
}

/* 12B: Fork için boş task (ready değil, schedule seçmez).
 * Kernel stack'i ayrılır, PID atanır, listeye eklenir. */
struct task* task_prepare(const char* name) {
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t) return 0;
    memset(t, 0, sizeof(struct task));
    fd_table_init(t);
    t->pid = next_pid++;
    t->pgid = t->pid;
    strncpy(t->name, name, TASK_NAME_LEN-1);
    t->name[TASK_NAME_LEN-1] = '\0';
    t->stack = (uint32_t*)kmalloc(TASK_STACK_SIZE);
    if (!t->stack) { kfree(t); return 0; }
    t->stack_top = (uint32_t)t->stack + TASK_STACK_SIZE;
    t->state = TASK_READY; /* fork finalize sonrası READY; schedule hemen değil */
    t->next = 0;
    /* PID benzersizliği: task_lock korunmasa da next_pid tek CPU'da yazılır
     * ama fork syscall kesmeleri kapalı; listeye eklemeyi lock ile yap. */
    uint32_t flags;
    spin_lock_irqsave(&task_lock, &flags);
    struct task* cur = task_list;
    while (cur->next) cur = cur->next;
    cur->next = t;
    spin_unlock_irqrestore(&task_lock, flags);
    return t;
}

/* 12B: Fork child için iret - jump_to_user_mode pattern'ine benzer.
 * 19A-FIX: TÜM genel register'lar geri yüklenir (fork_*, fork.c'de saklanır).
 * Eskiden sadece eip/esp/eflags kuruluyordu; ebx/ecx/edx/esi/edi/ebp çöp
 * kalıyordu (-O2 tesadüfen çalışıyordu, -O0 ebp çerçevesiyle anında fault).
 * Unix fork semantiği: çocuk tüm register'ların kopyasıyla başlar (eax=0 hariç).
 * Düzen: stash(ecx/edx değerleri) + 20B iret çerçevesi; iret'in esp slotu
 * orijinal esp'yi gösterir, stash ölü stack altında kalır. */
void enter_user_mode_fork(void) {
    struct task* t = current_task;
    uint32_t eip    = t->user_eip;
    uint32_t esp    = t->user_esp;
    /* H2 FIX: iret imajını sanitize et (fork sanitize'ını atlayan eski/
     * harici task'lar dahil hiçbir yoldan TF/DF/IOPL user'a sızmasın). */
    uint32_t eflags = (t->user_eflags ? t->user_eflags : 0x202u);
    eflags = (eflags & 0x8D7u) | 0x202u;
    t->user_eflags = eflags; /* depolanan kopyayı da temiz tut */
    /* Stash: iret çerçevesi 7 register'a sığmaz (9 canlı değer); fork_ecx/edx
     * user stack'in tepesine yazılır, çerçeve kurulduktan sonra geri yüklenir.
     * (Kernel VA'dan user stack'e doğrudan yazılabilir — signal_deliver deseni.) */
    uint32_t adj = esp - 8;
    ((volatile uint32_t*)adj)[0] = t->fork_ecx;
    ((volatile uint32_t*)adj)[1] = t->fork_edx;
    /* Segmentler önce (henüz hiçbir canlı değer register'da değil;
     * volatile bariyer C atamalarını sonra tutar). */
    asm volatile(
        "mov $0x23, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        ::: "eax", "memory");
    register uint32_t r_si asm("esi") = t->fork_esi;
    register uint32_t r_di asm("edi") = t->fork_edi;
    register uint32_t r_bx asm("ebx") = t->fork_ebx;
    register uint32_t r_bp asm("ebp") = t->fork_ebp;
    register uint32_t r_dx asm("edx") = eip;
    register uint32_t r_cx asm("ecx") = adj;
    register uint32_t r_ax asm("eax") = eflags;
    /* KÖK NEDEN FIX (H2): "m" operandı esp switch'inden SONRA user stack'ten
     * okurdu. Burada switch sonrası SADECE esp-relative (user stack'in kendisi)
     * ve sabitler kullanılır; kernel belleğine dokunulmaz. ebp'ye hiç
     * dokunulmaz (girdi olarak taşınır). Bu blok dönmez (iret). */
    asm volatile(
        "movl %%ecx, %%esp\n\t"   /* esp = adj */
        "leal 8(%%esp), %%ecx\n\t" /* ecx = orig esp (iret esp slotu için) */
        "pushl $0x23\n\t"          /* ss */
        "pushl %%ecx\n\t"          /* esp */
        "pushl %%eax\n\t"          /* eflags */
        "pushl $0x1B\n\t"          /* cs */
        "pushl %%edx\n\t"          /* eip */
        "movl 20(%%esp), %%ecx\n\t" /* stash'ten fork_ecx */
        "movl 24(%%esp), %%edx\n\t" /* stash'ten fork_edx */
        "xorl %%eax, %%eax\n\t"   /* fork() == 0 */
        "iret\n\t"
        :
        : "r"(r_si), "r"(r_di), "r"(r_bx), "r"(r_bp),
          "r"(r_dx), "r"(r_cx), "r"(r_ax)
        : "memory"
    );
    for(;;) asm volatile("hlt");
}

void fork_child_finalize(void) {
    struct task* t = current_task;
    serial_puts("[12B] child finalize pid="); serial_puthex(t->pid);
    serial_puts(" eip=0x"); serial_puthex(t->user_eip);
    serial_puts(" esp=0x"); serial_puthex(t->user_esp); serial_puts("\n");
    vga_puts("[12B] child finalize pid "); vga_putdec(t->pid); vga_puts("\n");
    enter_user_mode_fork();
    for(;;) asm volatile("hlt");
}

struct task* task_create(void (*entry)(void), const char* name) {
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t) return 0;
    memset(t, 0, sizeof(struct task));
    fd_table_init(t);
    t->pid = next_pid++;
    t->pgid = t->pid;
    strncpy(t->name, name, TASK_NAME_LEN-1);
    t->name[TASK_NAME_LEN-1] = '\0';
    t->stack = (uint32_t*)kmalloc(TASK_STACK_SIZE);
    if (!t->stack) { kfree(t); return 0; }
    t->stack_top = (uint32_t)t->stack + TASK_STACK_SIZE;

    // Stack'i hazırla: task_bootstrap'e ret ile atlanır, bootstrap entry'yi çağırır
    // Layout (low = esp): [edi][esi][ebx][ebp][task_bootstrap][entry]
    extern void task_bootstrap(void);
    uint32_t* stack_top = (uint32_t*)t->stack_top;
    *--stack_top = (uint32_t)entry;           // bootstrap için arg (pop eax)
    *--stack_top = (uint32_t)task_bootstrap;  // switch ret hedefi
    *--stack_top = 0; // ebp
    *--stack_top = 0; // ebx
    *--stack_top = 0; // esi
    *--stack_top = 0; // edi

    t->esp = (uint32_t)stack_top;
    t->ebp = 0;
    t->eip = (uint32_t)task_bootstrap;
    t->next = 0;

    // Listeye ekle (lock ile)
    uint32_t flags;
    spin_lock_irqsave(&task_lock, &flags);
    struct task* cur = task_list;
    while(cur->next) cur = cur->next;
    cur->next = t;
    spin_unlock_irqrestore(&task_lock, flags);

    vga_puts("[4A] Task create pid "); vga_putdec(t->pid);
    vga_puts(" name "); vga_puts(t->name);
    vga_puts(" esp=0x"); vga_puthex(t->esp); vga_puts("\n");
    serial_puts("[4A] Task create pid "); serial_puthex(t->pid);
    serial_puts(" esp=0x"); serial_puthex(t->esp); serial_puts("\n");

    // 11A-A: state = READY EN SON — stack tam hazırlandıktan sonra
    t->state = TASK_READY;

    return t;
}

/* User task (Ring 3) oluştur - kernel stack bootstrap'i enter_user_mode kullanır */
struct task* task_create_user(void (*entry)(void), const char* name) {
    struct task* t = (struct task*)kmalloc(sizeof(struct task));
    if (!t) return 0;
    memset(t, 0, sizeof(struct task));
    fd_table_init(t);
    t->pid = next_pid++;
    strncpy(t->name, name, TASK_NAME_LEN-1);
    t->state = TASK_READY;
    t->stack = (uint32_t*)kmalloc(TASK_STACK_SIZE);
    if (!t->stack) { kfree(t); return 0; }
    t->stack_top = (uint32_t)t->stack + TASK_STACK_SIZE;
    /* 12A: Her user task kendi address space'i - kernel map'li boş dizin */
    t->cr3 = paging_create_space();
    if (!t->cr3) { kfree(t->stack); kfree(t); return 0; }

    // Kernel stack: task_bootstrap -> enter_user_mode -> jump_to_user_mode(entry, user_esp)
    extern void task_bootstrap(void);
    // task_bootstrap, pop ettiği fonksiyonu call eder. O fonksiyon = enter_user_mode (kernel).
    // enter_user_mode, current_task->user_eip'e iret eder (Ring 3).
    uint32_t* stack_top = (uint32_t*)t->stack_top;
    *--stack_top = (uint32_t)enter_user_mode; // bootstrap call hedefi
    *--stack_top = (uint32_t)task_bootstrap;
    *--stack_top = 0; // ebp
    *--stack_top = 0; // ebx
    *--stack_top = 0; // esi
    *--stack_top = 0; // edi

    t->esp = (uint32_t)stack_top;
    t->ebp = 0;
    t->eip = (uint32_t)task_bootstrap;
    t->next = 0;

    // User alanları - caller task_set_user ile user_esp/eip/stack kuracak
    // Burada varsayılan: user eip = entry (caller iret hedefi olarak verir)
    t->user_eip = (uint32_t)entry;
    t->user_cs = 0x1B;
    t->user_ss = 0x23;
    t->user_eflags = 0x202;
    t->user_esp = 0;

    // Listeye ekle (lock ile)
    uint32_t flags;
    spin_lock_irqsave(&task_lock, &flags);
    struct task* cur = task_list;
    while(cur->next) cur = cur->next;
    cur->next = t;
    spin_unlock_irqrestore(&task_lock, flags);

    vga_puts("[6B] User task create pid "); vga_putdec(t->pid);
    vga_puts(" name "); vga_puts(t->name); vga_puts("\n");
    serial_puts("[6B] User task create pid "); serial_puthex(t->pid);
    serial_puts(" name "); serial_puts(t->name); serial_puts("\n");

    return t;
}

void task_exit(void) {
    // 12D: Kapanan task'in tüm açık fd'lerini kapat (pipe'lar dahil)
    // chfs ve pipe katmanı task_current()->fd_table üzerinden çalışır
    extern int fs_close(int fd);
    for (int i = 0; i < 16; i++) {
        if (current_task->fd_table[i].valid) {
            fs_close(i);
        }
    }
    // 12H: waitpid için parent'ı uyandır
    extern void wait_wakeup_parent(struct task* child);
    wait_wakeup_parent(current_task);
    uint32_t flags;
    spin_lock_irqsave(&task_lock, &flags);
    current_task->state = TASK_DEAD;
    vga_puts("[4A] Task exit pid "); vga_putdec(current_task->pid); vga_puts("\n");
    serial_puts("[4A] Task exit pid "); serial_puthex(current_task->pid); serial_puts("\n");
    spin_unlock_irqrestore(&task_lock, flags);
    // Bir sonraki task'e geç
    schedule();
    // Buraya dönmemeli
    for(;;) asm volatile("hlt");
}

void task_yield(void) {
    schedule();
}

/* 11B: CPU ID - fiziksel LAPIC ID'den 0..MAX_CPU-1'e kilitle */
int cpu_id_get(void) {
    // Fiziksel LAPIC ID'den 0..MAX_CPU-1 aralığına kilitle.
    // AP'de IA32_APIC_BASE MSR erken 0 dönebilir; mmio LAPIC_ID registrı güvenlidir.
    extern uint32_t apic_get_raw_id(void);
    uint32_t lapic_id = apic_get_raw_id();
    if (lapic_id >= MAX_CPU) lapic_id = 0;
    return (int)lapic_id;
}

void cpu_id_init(void) {
    for (int i = 0; i < MAX_CPU; i++) {
        current_per_cpu[i] = (i == 0) ? current_task : 0;
    }
}

/* 13D: MLQ scheduler - 4 seviye (0 idle, 1 high, 2 normal, 3 low) */
void schedule(void) {
    if (!current_per_cpu[0] && !current_task) return;
    int cpu = cpu_id_get();
    /* 13F+SMP: 12A-13F suite'i BSP-ince dizilidir; AP'de task switch suite'i
       bozar. Per-CPU TSS slotu hazırdır (0x30) ama şimdilik BSP-only:
       AP yalnızca online kalır (IPI/TLB). */
    if (cpu != 0) return;
    struct task* cur_task = current_per_cpu[cpu] ? current_per_cpu[cpu] : current_task;
    static int sched_count[MAX_CPU] = {0};
    uint32_t flags;
    spin_lock_irqsave(&rq_lock[cpu], &flags);

    // 13D: time slice - eğer cur hala RUNNING ve ticks_left >0 ise devam et
    if (cur_task->state == TASK_RUNNING && cur_task->ticks_left > 0) {
        cur_task->ticks_left--;
        if (cur_task->ticks_left > 0) {
            spin_unlock_irqrestore(&rq_lock[cpu], flags);
            return;
        }
        // slice bitti, yeniden kuyruğa koy
        cur_task->ticks_left = (cur_task->priority + 1) * 5;
        if (cur_task->priority < 3) cur_task->priority++; // aging: düşük önceliğe doğru
    }

    // MLQ: önce yüksek öncelik (1), sonra normal (2), sonra düşük (3)
    // 12C/starvation kök nedeni: her zaman liste başından tarama, liste başındaki
    // main task'ın aynı priodaki diğerlerini aç bırakmasıydı. Per-CPU round-robin
    // cursor ile her prio seviyesinde son seçilenin sonrasından başla.
    static struct task* rr_cursor[MAX_CPU] = {0};
    // Cursor reap edilmiş (listeden çıkmış) olabilir; doğrula
    if (rr_cursor[cpu]) {
        struct task* v = task_list;
        while (v && v != rr_cursor[cpu]) v = v->next;
        if (!v) rr_cursor[cpu] = 0;
    }
    struct task* next = 0;
    for (int prio = 1; prio <= 3 && !next; prio++) {
        // 1. tur: cursor sonrasından liste sonuna
        struct task* t = rr_cursor[cpu] ? rr_cursor[cpu]->next : task_list;
        if (!t) t = task_list;
        while (t) {
            if (t->state == TASK_READY && t->priority == prio) { next = t; break; }
            t = t->next;
        }
        // 2. tur: liste başından cursor'a kadar (cursor dahil)
        if (!next) {
            t = task_list;
            struct task* stop = rr_cursor[cpu] ? rr_cursor[cpu]->next : 0;
            while (t && t != stop) {
                if (t->state == TASK_READY && t->priority == prio) { next = t; break; }
                t = t->next;
            }
        }
    }
    if (next) rr_cursor[cpu] = next;
    if (!next) {
        // Hiç high/normal/low yoksa idle
        struct task* t = task_list;
        while (t) { if (t->state == TASK_READY) { next = t; break; } t = t->next; }
    }
    if (!next || next->state != TASK_READY) {
        next = idle_task;
        if (next->state != TASK_READY && next->state != TASK_RUNNING) next->state = TASK_READY;
    }
    // Seçilen next için ticks_left ayarla
    if (next->ticks_left <= 0) next->ticks_left = (next->priority + 1) * 5;

    if (sched_count[cpu] < 20 || next->pid >= 10) {
        serial_puts("[13D] sched cpu="); serial_puthex(cpu);
        serial_puts(" cur="); serial_puthex(cur_task->pid); serial_puts("("); serial_puthex(cur_task->priority); serial_puts(")");
        serial_puts(" next="); serial_puthex(next->pid); serial_puts("("); serial_puthex(next->priority); serial_puts("/"); serial_puthex(next->state); serial_puts(") tick="); serial_puthex(timer_get_ticks()); serial_puts("\n");
        if (sched_count[cpu] < 20) sched_count[cpu]++;
    }

    if (next == cur_task) {
        spin_unlock_irqrestore(&rq_lock[cpu], flags);
        return;
    }

    struct task* prev = cur_task;
    prev->state = (prev->state == TASK_RUNNING) ? TASK_READY : prev->state;
    next->state = TASK_RUNNING;
    current_per_cpu[cpu] = next;
    current_task = next;

    // TSS stack güncelle (per-CPU TSS slot - BSP slot 0x28)
    tss_set_stack_cpu(cpu, 0x10, next->stack_top);

    // 12A: Address space switch (per-task CR3)
    uint32_t target_cr3 = next->cr3 ? next->cr3 : (uint32_t)page_directory;
    extern void paging_switch_space(uint32_t cr3);
    paging_switch_space(target_cr3);

    // 12G: signal delivery - CR3 hedefe geçtikten SONRA (next'in user stack'i
    // mevcut space'te map'li). Pending signal varsa handler'a yönlendir.
    if (next->sig_pending) {
        for (int s = 0; s < 32; s++) {
            if ((next->sig_pending & (1u << s)) && next->sig_handlers[s]) {
                uint32_t handler = next->sig_handlers[s];
                next->sig_pending &= ~(1u << s);
                uint32_t new_esp = next->user_esp - 32;
                /* Guard: hedef user stack map'li değilse frame yazma
                 * (ring-0 fault = kilitlenme). Sinyal düşer, task yaşar. */
                extern uint32_t paging_get_phys(uint32_t virt);
                if (!paging_get_phys(new_esp) || !paging_get_phys(next->user_esp - 1)) break;
                uint32_t* stack = (uint32_t*)new_esp;
                stack[0] = next->user_eip;
                stack[1] = next->user_esp;
                stack[2] = next->user_eflags;
                stack[3] = s; // sig num
                next->user_esp = new_esp;
                next->user_eip = handler;
                break;
            }
        }
    }

    // Lock'ı bırak ama interrupt'lar hala kapalı kalsın - switch sırasında
    spin_unlock(&rq_lock[cpu]);

    // Context switch (interrupts disabled)
    task_switch(&prev->esp, &next->esp);

    // Buraya eski task geri döndüğünde gelir
    irq_restore(flags);
}

void task_dump(void) {
    vga_puts("PID  NAME           STATE\n");
    vga_puts("---  -------------  -----\n");
    serial_puts("PID NAME STATE\n");
    struct task* t = task_list;
    while(t) {
        vga_putdec(t->pid); vga_puts("   ");
        vga_puts(t->name);
        for(int i=strlen(t->name); i<14; i++) vga_putc(' ');
        const char* s = "UNKNOWN";
        switch(t->state){
            case TASK_READY: s="READY"; break;
            case TASK_RUNNING: s="RUNNING"; break;
            case TASK_BLOCKED: s="BLOCKED"; break;
            case TASK_DEAD: s="DEAD"; break;
        }
        vga_puts(s); vga_puts("\n");
        serial_puts("pid="); serial_puthex(t->pid); serial_puts(" "); serial_puts(t->name);
        serial_puts(" "); serial_puts(s); serial_puts("\n");
        t = t->next;
        if (!t) break;
    }
}