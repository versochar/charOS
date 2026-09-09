#ifndef CHAROS_CORE_SPINLOCK_H
#define CHAROS_CORE_SPINLOCK_H

#include <stdint.h>

typedef struct {
    volatile uint32_t locked;
} spinlock_t;

#define SPINLOCK_INIT {0}

void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);
int  spin_trylock(spinlock_t* lock);
void spin_lock_irqsave(spinlock_t* lock, uint32_t* flags);
void spin_unlock_irqrestore(spinlock_t* lock, uint32_t flags);

static inline uint32_t irq_save(void) {
    uint32_t flags;
    asm volatile("pushf; pop %0" : "=r"(flags));
    asm volatile("cli");
    return flags;
}
static inline void irq_restore(uint32_t flags) {
    asm volatile("push %0; popf" : : "r"(flags) : "memory", "cc");
}

#endif