#ifndef CHAROS_PROCESS_TASK_H
#define CHAROS_PROCESS_TASK_H

#include <stdint.h>
#include <process/sandbox.h>
#include <memory/paging.h>
#include <fs/fd.h>
#include <core/spinlock.h>

#define TASK_MAX        32
#define MAX_CPU         4
#define TASK_STACK_SIZE 8192
#define TASK_NAME_LEN   32

enum task_state {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DEAD
};

struct task {
    int pid;
    char name[TASK_NAME_LEN];
    enum task_state state;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;
    uint32_t *stack;          // kernel stack bottom (kmalloc)
    uint32_t stack_top;       // kernel stack top (high address)
    struct task* next;
    int ticks;                // for scheduler
    int priority;

    /* 6B: User mode alanları */
    uint32_t user_esp;        // user stack pointer
    uint32_t user_eip;        // user program entry
    uint32_t user_eflags;     // user eflags
    uint32_t user_cs;         // user code segment (0x1B)
    uint32_t user_ss;         // user stack segment (0x23)
    uint32_t user_stack_addr; // user stack sanal adresi

    /* 12A: Per-task address space */
    uint32_t cr3;             // page directory fizikseli (0 = kernel default)
    struct task_region user_regions[TASK_MAX_REGIONS];
    int user_region_count;

    /* 12B: Fork için kaydedilmiş general registers (iret harici) */
    uint32_t fork_edi, fork_esi, fork_ebp, fork_ebx, fork_edx, fork_ecx;

    /* 12D: Per-process file descriptor table */
    struct fd_entry fd_table[16];

    /* 12E: sleep */
    uint32_t sleep_until;

    /* 12G: parent for waitpid */
    int parent_pid;

    /* 12G: signals */
    uint32_t sig_handlers[32];
    uint32_t sig_pending;
    uint32_t sig_mask;
    uint32_t sig_stack;

    /* 13C: mmap VMAs */
    struct vm_area* vmas;

    /* 13D: priority */
    int ticks_left;

    /* 14A: user heap program break (brk_base==0 ise tanımsız) */
    uint32_t brk_base;
    uint32_t brk_cur;

    /* 14E: sahiplik */
    int uid;
    int gid;

    /* 20C: ek gruplar + capability bitmask (öbür izinler) */
#define TASK_NGROUPS 8
    int groups[TASK_NGROUPS];
    int ngroups;
    uint32_t caps;

    /* 23.4: görev sandbox filtresi (varsayılan: kapalı = her şey serbest) */
    uint32_t sb_mask[SB_MASK_WORDS];
    int sb_on;

    /* 13E: exit status, thread/process mode, process group */
    int exit_status;
    int is_thread;      /* 13F: shared address space */
    int pgid;
    int home_cpu;       /* 13F+SMP: bu task hangi CPU'da çalışır (0=BSP, 1=AP) */
    uint32_t user_tls;  /* 15C: per-task TLS tabanı (SYS_TLS_SET/GET) */
};

void task_init(void);
struct task* task_create(void (*entry)(void), const char* name);
struct task* task_create_user(void (*entry)(void), const char* name); // 6B: ileride ELF için
struct task* task_prepare(const char* name); // 12B: fork için boş task (ready değil)
void task_add_region(struct task* t, uint32_t base, uint32_t pages); // 12A
void fd_table_copy(struct task* dst, struct task* src); // 12D
void task_exit(void);
void task_yield(void);
void task_exit_and_switch(void); // 6B: user task exit
void schedule(void);
void switch_to(struct task* next);
struct task* task_current(void);
void task_dump(void);
void task_set_user(uint32_t esp, uint32_t eip, uint32_t cs, uint32_t ss);

/* 6B: Gömülü user programını yükle ve Ring 3 task oluştur */
struct task* user_launch(void);

/* 12B: Fork child finalize (kernel stack -> iret) */
void fork_child_finalize(void);

extern struct task* current_task;
extern struct task* current_per_cpu[MAX_CPU];
extern struct task* task_list;
extern spinlock_t task_lock;

/* 11B: Per-CPU scheduler için */
void cpu_id_init(void);
int cpu_id_get(void);

#endif