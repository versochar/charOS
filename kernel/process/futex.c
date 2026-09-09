#include <process/futex.h>
#include <process/task.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14D: Futex bekleme kuyrukları. Tek CPU (BSP) varsayımıyla kilit altında
 * oku-blokla/uyandır; uyanan görevin değeri tekrar denetlemesi gerekir. */

struct futex_slot {
    int used;
    uint32_t uaddr;
    uint32_t asid; /* adres alanı (cr3), yoksa 0 */
    struct task* waiter;
};

static struct futex_slot fut_slots[FUTEX_MAX];
static spinlock_t futex_lock = SPINLOCK_INIT;
static int futex_on = 0;

int futex_init(void) {
    memset(fut_slots, 0, sizeof(fut_slots));
    futex_lock.locked = 0;
    futex_on = 1;
    return 0;
}

/* Kullanıcı sözcüğünü güvenli oku (map'siz adrese dokunma) */
static int futex_get_user(uint32_t uaddr, uint32_t* out) {
    extern uint32_t paging_get_phys(uint32_t virt);
    if (!uaddr) return -1;
    if (!paging_get_phys(uaddr)) return -1;
    if (!paging_get_phys(uaddr + 3)) return -1;
    *out = *(volatile uint32_t*)uaddr;
    return 0;
}

int futex_wait(uint32_t uaddr, uint32_t val) {
    if (!futex_on || !uaddr) return -1;
    struct task* cur = task_current();
    if (!cur) return -1;
    uint32_t curval = 0;
    uint32_t flags;
    spin_lock_irqsave(&futex_lock, &flags);
    if (futex_get_user(uaddr, &curval) != 0) {
        spin_unlock_irqrestore(&futex_lock, flags);
        return -1;
    }
    if (curval != val) {
        spin_unlock_irqrestore(&futex_lock, flags);
        return -1; /* EAGAIN eşdeğeri: değer değişmiş, bloklanma */
    }
    int i;
    for (i = 0; i < FUTEX_MAX; i++)
        if (!fut_slots[i].used) break;
    if (i >= FUTEX_MAX) {
        spin_unlock_irqrestore(&futex_lock, flags);
        return -1;
    }
    fut_slots[i].used = 1;
    fut_slots[i].uaddr = uaddr;
    fut_slots[i].asid = cur->cr3;
    fut_slots[i].waiter = cur;
    cur->state = TASK_BLOCKED;
    spin_unlock_irqrestore(&futex_lock, flags);
    schedule();
    return 0; /* uyandırıldı */
}

int futex_wake(uint32_t uaddr, int n) {
    if (!futex_on || !uaddr || n <= 0) return n <= 0 ? -1 : 0;
    if (n > FUTEX_MAX) n = FUTEX_MAX;
    struct task* cur = task_current();
    uint32_t asid = cur ? cur->cr3 : 0;
    int woken = 0;
    uint32_t flags;
    spin_lock_irqsave(&futex_lock, &flags);
    for (int i = 0; i < FUTEX_MAX && woken < n; i++) {
        if (fut_slots[i].used && fut_slots[i].uaddr == uaddr &&
            fut_slots[i].asid == asid && fut_slots[i].waiter) {
            fut_slots[i].waiter->state = TASK_READY;
            fut_slots[i].waiter = 0;
            fut_slots[i].used = 0;
            woken++;
        }
    }
    spin_unlock_irqrestore(&futex_lock, flags);
    return woken;
}

int futex_selftest(void) {
    int ok = 1;
    if (!futex_on && futex_init() != 0) return -1;
    /* Eşleşmeyen değer: bloklanmadan -1 */
    static volatile uint32_t cell = 0;
    /* Not: kernel adresi paging_get_phys'ten geçer (identity map) */
    cell = 7;
    if (futex_wait((uint32_t)&cell, 8) != -1) ok = 0;
    /* Boş adresten uyandırma: 0 döner */
    if (futex_wake(0x12345000u, 1) != 0) ok = 0;
    if (futex_wait(0, 0) != -1) ok = 0;
    if (futex_wake((uint32_t)&cell, 0) != -1) ok = 0;
    if (ok) {
        serial_puts("[14D] futex fastpath [PASS]\n");
        vga_puts("[14D] futex fastpath [PASS]\n");
        return 0;
    }
    serial_puts("[14D] futex [FAIL]\n");
    vga_puts("[14D] futex [FAIL]\n");
    return -1;
}
