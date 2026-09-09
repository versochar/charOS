#include <process/wait.h>
#include <process/task.h>
#include <memory/kheap.h>
#include <drivers/serial.h>
#include <core/spinlock.h>

extern struct task* task_list;
extern spinlock_t task_lock;

/* 13E: wait_task(pid) - child'ın DEAD olmasını bekle, status + reaping:
 *  - deadlock riskini azaltmak için önce child var mı kontrol edilir
 *  - bulunamazsa ve hiç child yoksa -1 döner
 *  - bulunursa DEAD task listeden çıkarılır, stack/task struct kfree edilir
 *  (paging space frame'leri şimdilik sızıntı yapar - ileride paging_free_space)
 */
int wait_task(int pid, int* status) {
    // 12E: sleep_until ile aynı per-CPU çözümü kullan (global current_task SMP'de
    // çalışan CPU'nun task'ı olmayabilir).
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur) return -1;
    for (;;) {
        uint32_t flags;
        spin_lock_irqsave(&task_lock, &flags);
        struct task* t = task_list;
        struct task* prev = 0;
        while (t) {
            if (t->state == TASK_DEAD && t->parent_pid == cur->pid &&
                (pid == -1 || t->pid == pid)) {
                int ret = t->pid;
                if (status) *status = t->exit_status;
                if (prev) prev->next = t->next; else task_list = t->next;
                spin_unlock_irqrestore(&task_lock, flags);
                if (t->stack) kfree(t->stack);
                kfree(t);
                return ret;
            }
            prev = t;
            t = t->next;
        }
        // Alive child var mı?
        int has_child = 0;
        for (t = task_list; t; t = t->next) {
            if (t->parent_pid == cur->pid) { has_child = 1; break; }
        }
        if (!has_child) {
            spin_unlock_irqrestore(&task_lock, flags);
            return -1; /* no children */
        }
        spin_unlock_irqrestore(&task_lock, flags);
        cur->state = TASK_BLOCKED;
        schedule();
    }
}

int waitpid(int pid, int* status, int options) {
    (void)options;
    return wait_task(pid, status);
}

/* 19G: WNOHANG yoklaması — tek tur: ölü eşleşen child varsa biç (reap),
 * yoksa ama canlı child varsa 0, hiç child yoksa -1. Asla bloklanmaz. */
int wait_task_wnohang(int pid, int* status) {
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    if (!cur) return -1;
    uint32_t flags;
    spin_lock_irqsave(&task_lock, &flags);
    struct task* t = task_list;
    struct task* prev = 0;
    while (t) {
        if (t->state == TASK_DEAD && t->parent_pid == cur->pid &&
            (pid == -1 || t->pid == pid)) {
            int ret = t->pid;
            if (status) *status = t->exit_status;
            if (prev) prev->next = t->next; else task_list = t->next;
            spin_unlock_irqrestore(&task_lock, flags);
            if (t->stack) kfree(t->stack);
            kfree(t);
            return ret;
        }
        prev = t;
        t = t->next;
    }
    for (t = task_list; t; t = t->next) {
        if (t->parent_pid == cur->pid && (pid == -1 || t->pid == pid)) {
            spin_unlock_irqrestore(&task_lock, flags);
            return 0; /* canlı child var, bloklanmadan dön */
        }
    }
    spin_unlock_irqrestore(&task_lock, flags);
    return -1; /* böyle bir child yok */
}

/* 12H/13E: child öldüğünde parent'ı uyandır (task_exit çağırır) */
void wait_wakeup_parent(struct task* child) {
    struct task* t = task_list;
    while (t) {
        // 12E: sleep_until set olan task (nanosleep uykusu) child exit'inden
        // etkilenmemeli; yalnızca wait()'te bekleyen parent'ları uyandır.
        if (t->state == TASK_BLOCKED && t->sleep_until == 0 &&
            t->pid == child->parent_pid) {
            t->state = TASK_READY;
            // task'taki tüm blocked parentları uyandır
        }
        t = t->next;
    }
}
