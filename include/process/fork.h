#ifndef CHAROS_PROCESS_FORK_H
#define CHAROS_PROCESS_FORK_H

#include <process/task.h>
#include <core/isr.h>

/* 12B: Gerçek fork - parent'ın address space ve register state'ini kopyala.
 * regs: int 0x80 trap frame (syscall'dan gelir: eip/cs/eflags/useresp/ss içerir).
 * Child aynı noktadan eax=0 ile döner, parent child pid döner. */
struct task* fork_task(struct registers* regs);

/* Eski imza uyumu (kullanılmıyor) için forward: */
static inline struct task* fork_task_no_regs(void) { return 0; }

#endif
