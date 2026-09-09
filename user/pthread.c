/* 15B: pthread-lite (SYS_CLONE + SYS_FUTEX üstünde). */
#include "pthread.h"
#include "dl.h"

#define SYS_EXIT 0
#define SYS_FORK 12
#define SYS_WAITPID 19
#define SYS_MMAP 90
#define SYS_MUNMAP 91
#define SYS_CLONE 130
#define SYS_FUTEX 139
#define SYS_YIELD 4
#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

#define PTHREAD_STACK (8 * 1024)
#define PTHREAD_MAX 8

static unsigned p_syscall(unsigned n, unsigned a, unsigned b, unsigned c) {
    unsigned ret;
    asm volatile("int $0x80"
        : "=a"(ret) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory");
    return ret;
}

/* Slot tahsisi kilidi (kısa kritik bölge, bloklama yok) */
static volatile int create_lock = 0;
static void spin_lock(volatile int* l) {
    while (__sync_lock_test_and_set((int*)l, 1)) { }
}
static void spin_unlock(volatile int* l) {
    __sync_lock_release((int*)l);
}

struct start_info {
    pthread_fn fn;
    void* arg;
    void* retval;
    int tid;
    unsigned stack;
};

static struct start_info slots[PTHREAD_MAX];

/* Trambolin yığınından kendi slotunu bulur: el sıkışmasız, yarışsız.
 * (Eski active_info/info_copied tasarımı yavaş ilk-zamanlamada iki
 * thread'i aynı bilgiye düşürüyordu.) */
static void pthread_trampoline(void) {
    unsigned sp;
    asm volatile("movl %%esp, %0" : "=r"(sp));
    int slot = -1;
    for (int i = 0; i < PTHREAD_MAX; i++) {
        if (slots[i].fn != 0 && sp >= slots[i].stack &&
            sp < slots[i].stack + PTHREAD_STACK) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        p_syscall(SYS_EXIT, 3, 0, 0);
        for (;;) { }
    }
    /* 15C: her thread kendi TLS bloğunu kurar (main bloğundan bağımsız) */
    dl_tls_new();
    void* r = slots[slot].fn(slots[slot].arg);
    slots[slot].retval = r;
    p_syscall(SYS_EXIT, (unsigned)(int)r, 0, 0);
    for (;;) { }
}

int pthread_create(pthread_t* th, void* attr, pthread_fn fn, void* arg) {
    (void)attr;
    if (!th || !fn) return -1;
    spin_lock(&create_lock);
    int slot = -1;
    for (int i = 0; i < PTHREAD_MAX; i++)
        if (slots[i].fn == 0) { slot = i; break; }
    if (slot < 0) { spin_unlock(&create_lock); return -1; }
    unsigned stack = p_syscall(SYS_MMAP, 0, PTHREAD_STACK,
        PROT_READ | PROT_WRITE | ((MAP_PRIVATE | MAP_ANONYMOUS) << 8));
    if (stack == 0xFFFFFFFFu || stack == 0) {
        spin_unlock(&create_lock);
        return -1;
    }
    slots[slot].fn = fn;
    slots[slot].arg = arg;
    slots[slot].retval = 0;
    slots[slot].tid = 0;
    slots[slot].stack = stack;
    unsigned tid = p_syscall(SYS_CLONE, (unsigned)pthread_trampoline,
                             stack + PTHREAD_STACK, 0);
    if (tid == 0xFFFFFFFFu) {
        p_syscall(SYS_MUNMAP, stack, PTHREAD_STACK, 0);
        slots[slot].fn = 0;
        spin_unlock(&create_lock);
        return -1;
    }
    slots[slot].tid = (int)tid;
    spin_unlock(&create_lock);
    *th = (int)tid;
    return 0;
}

int pthread_join(pthread_t th, void** retval) {
    unsigned st = 0;
    unsigned r = p_syscall(SYS_WAITPID, (unsigned)th, (unsigned)&st, 0);
    if ((int)r < 0) return -1;
    if (retval) *retval = (void*)st;
    /* Slotu tid ile bul, yığını iade et, serbest bırak */
    for (int i = 0; i < PTHREAD_MAX; i++) {
        if (slots[i].fn != 0 && slots[i].tid == th) {
            if (slots[i].stack)
                p_syscall(SYS_MUNMAP, slots[i].stack, PTHREAD_STACK, 0);
            slots[i].fn = 0;
            slots[i].tid = 0;
            slots[i].stack = 0;
            break;
        }
    }
    return 0;
}

/* --- mutex: 0 açık, 1 kilitli, 2 çekişmeli --- */
int pthread_mutex_init(pthread_mutex_t* m, void* attr) {
    (void)attr;
    if (!m) return -1;
    m->state = 0;
    return 0;
}

int pthread_mutex_lock(pthread_mutex_t* m) {
    if (!m) return -1;
    int c = __sync_val_compare_and_swap(&m->state, 0, 1);
    if (c == 0) return 0;
    while (1) {
        if (c == 2 || __sync_val_compare_and_swap(&m->state, 1, 2) != 0) {
            p_syscall(SYS_FUTEX, (unsigned)&m->state, 0, 2);
        }
        c = __sync_val_compare_and_swap(&m->state, 0, 2);
        if (c == 0) return 0;
    }
}

int pthread_mutex_unlock(pthread_mutex_t* m) {
    if (!m) return -1;
    int prev = __sync_lock_test_and_set(&m->state, 0);
    (void)prev;
    /* Bekleyen olabilir: her zaman birini uyandır (ucuz ve doğru) */
    p_syscall(SYS_FUTEX, (unsigned)&m->state, 1, 1);
    return 0;
}

/* --- condvar: sıra sayaçlı --- */
int pthread_cond_init(pthread_cond_t* c, void* attr) {
    (void)attr;
    if (!c) return -1;
    c->seq = 0;
    return 0;
}

int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m) {
    if (!c || !m) return -1;
    unsigned s = c->seq;
    pthread_mutex_unlock(m);
    p_syscall(SYS_FUTEX, (unsigned)&c->seq, 0, s);
    pthread_mutex_lock(m);
    return 0;
}

int pthread_cond_signal(pthread_cond_t* c) {
    if (!c) return -1;
    c->seq++;
    p_syscall(SYS_FUTEX, (unsigned)&c->seq, 1, 1);
    return 0;
}

int pthread_cond_broadcast(pthread_cond_t* c) {
    if (!c) return -1;
    c->seq++;
    p_syscall(SYS_FUTEX, (unsigned)&c->seq, 1, 16);
    return 0;
}
