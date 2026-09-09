#include <process/signal.h>
#include <process/task.h>
#include <memory/paging.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

static int pending_sig = 0;

void signal_init(void) {
    pending_sig = 0;
    // Tüm task'ların sig handler'larını sıfırla (task_init sonrası çağrılır, task_list henüz boş olabilir)
    vga_puts("[9A] Signal init\n");
    serial_puts("[9A] Signal init\n");
}

void signal_send(int pid, int sig) {
    if (sig < 0 || sig >= SIG_MAX) return;
    extern void wait_wakeup_parent(struct task* child);
    extern void task_exit(void);
    int cpu = cpu_id_get();
    struct task* cur = current_per_cpu[cpu] ? current_per_cpu[cpu] : task_current();
    struct task* t = task_list;
    while (t) {
        if (t->pid == pid || pid == -1) { // -1: broadcast
            if (t->sig_handlers[sig]) {
                t->sig_pending |= (1u << sig);
                if (t->state == TASK_BLOCKED) t->state = TASK_READY;
            } else {
                // 12H: default action (handler yok) = terminate.
                // Shell gibi handler kurmayan task'lar kill ile ölür,
                // yoksa parent waitpid'de sonsuz bekler.
                if (t == cur) {
                    task_exit(); // dönmez
                    return;
                } else if (t->state != TASK_DEAD) {
                    /* 19G: ölü task'in fd'leri kapatılmazsa pipe yazma uçları
                     * açık kalır, okuyucular EOF yerine sonsuz bloklanır. */
                    extern void fs_close_table(struct fd_entry* tbl);
                    fs_close_table(t->fd_table);
                    t->state = TASK_DEAD;
                    t->exit_status = 128 + sig;
                    t->sig_pending &= ~(1u << sig);
                    wait_wakeup_parent(t);
                }
            }
            vga_puts("[12G] Signal sent "); vga_putdec(sig); vga_puts(" to pid "); vga_putdec(pid); vga_puts("\n");
            serial_puts("[12G] Signal "); serial_puthex(sig); serial_puts(" -> pid "); serial_puthex(pid); serial_puts("\n");
            if (pid != -1) break;
        }
        t = t->next;
    }
    if (pid == -1) {
        pending_sig = sig; // legacy
    }
}

int signal_check(void) {
    int s = pending_sig;
    pending_sig = 0;
    return s;
}

int sys_signal_handler(int sig, uint32_t handler) {
    if (sig < 0 || sig >= SIG_MAX) return -1;
    struct task* cur = task_current();
    if (!cur) return -1;
    cur->sig_handlers[sig] = handler;
    return 0;
}

int sys_kill_handler(int pid, int sig) {
    signal_send(pid, sig);
    return 0;
}

int sys_sigreturn_handler(void) {
    // Basit: signal frame'i user stack'ten pop et ve eski regs'i geri yükle
    // Bu, signal_deliver'da user stack'e itilen frame'i geri alır
    struct task* cur = task_current();
    if (!cur) return -1;
    // User stack'ten eski eip/esp/eflags/cs/ss'yi geri yükle
    // Bu, signal handler'ın ret'inden sonra çağrılır
    // Basit: sadece pending'i temizle ve devam et
    return 0;
}

int signal_has_pending(struct task* t) {
    if (!t) return 0;
    for (int i = 0; i < SIG_MAX; i++) {
        if ((t->sig_pending & (1u << i)) && t->sig_handlers[i]) return 1;
    }
    return 0;
}

void signal_deliver(struct task* t, struct registers* regs) {
    if (!t || !regs) return;
    for (int sig = 0; sig < SIG_MAX; sig++) {
        if ((t->sig_pending & (1u << sig)) && t->sig_handlers[sig]) {
            uint32_t handler = t->sig_handlers[sig];
            t->sig_pending &= ~(1u << sig);
            // User stack'e signal frame kur: eski eip/cs/eflags/ss/esp'yi sakla, handler'a atla
            // Basit: user_esp'yi 16 byte aşağı al, oraya eski değerleri yaz
            uint32_t new_esp = t->user_esp - 32;
            // User stack'e eski context'i yaz (eip, eflags, esp, cs, ss)
            // Paging'e erişim için scratch kullan
            // Basit: doğrudan user_esp adresine yaz (paging'e erişim var)
            // Eski değerleri stack'e it
            uint32_t* stack = (uint32_t*)new_esp;
            // Stack'e yazmadan önce sayfaların map'li olduğunu doğrula
            // Basit: direk yaz (user stack zaten map'li)
            stack[0] = regs->eip;
            stack[1] = regs->cs;
            stack[2] = regs->eflags;
            stack[3] = regs->useresp;
            stack[4] = regs->ss;
            stack[5] = (uint32_t)sig; // arg
            stack[6] = 0x1500000; // sigreturn trampoline adresi (ileride)
            stack[7] = handler; // ret addr için
            t->user_esp = new_esp;
            regs->eip = handler;
            regs->useresp = new_esp;
            vga_puts("[12G] deliver sig "); vga_putdec(sig); vga_puts(" to pid "); vga_putdec(t->pid); vga_puts("\n");
            serial_puts("[12G] deliver "); serial_puthex(sig); serial_puts("\n");
            break;
        }
    }
}
