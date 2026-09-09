#include <core/spinlock.h>

void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        // spin
        asm volatile("pause");
    }
}
void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->locked);
}
int spin_trylock(spinlock_t* lock) {
    return __sync_lock_test_and_set(&lock->locked, 1) == 0;
}
void spin_lock_irqsave(spinlock_t* lock, uint32_t* flags) {
    *flags = irq_save();
    spin_lock(lock);
}
void spin_unlock_irqrestore(spinlock_t* lock, uint32_t flags) {
    spin_unlock(lock);
    irq_restore(flags);
}