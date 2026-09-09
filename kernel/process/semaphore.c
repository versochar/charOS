#include <process/semaphore.h>
#include <process/task.h>
#include <string.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

static struct semaphore sems[SEM_MAX];

void sem_init_all(void) {
    memset(sems, 0, sizeof(sems));
    for (int i = 0; i < SEM_MAX; i++) sems[i].lock.locked = 0;
}

int sem_create(int value) {
    for (int i = 0; i < SEM_MAX; i++) {
        uint32_t flags;
        spin_lock_irqsave(&sems[i].lock, &flags);
        if (!sems[i].valid) {
            sems[i].valid = 1;
            sems[i].value = value;
            sems[i].wait_count = 0;
            memset(sems[i].wait_queue, 0, sizeof(sems[i].wait_queue));
            spin_unlock_irqrestore(&sems[i].lock, flags);
            serial_puts("[12F] sem_create id="); serial_puthex(i); serial_puts(" val="); serial_puthex(value); serial_puts("\n");
            return i;
        }
        spin_unlock_irqrestore(&sems[i].lock, flags);
    }
    return -1;
}

int sem_wait_id(int sem_id) {
    if (sem_id < 0 || sem_id >= SEM_MAX) return -1;
    struct semaphore* s = &sems[sem_id];
    while (1) {
        uint32_t flags;
        spin_lock_irqsave(&s->lock, &flags);
        if (!s->valid) { spin_unlock_irqrestore(&s->lock, flags); return -1; }
        if (s->value > 0) {
            s->value--;
            spin_unlock_irqrestore(&s->lock, flags);
            return 0;
        }
        // block
        if (s->wait_count >= 16) { spin_unlock_irqrestore(&s->lock, flags); return -1; }
        s->wait_queue[s->wait_count++] = task_current();
        task_current()->state = TASK_BLOCKED;
        spin_unlock_irqrestore(&s->lock, flags);
        schedule();
        // woken, loop to try again
    }
}

int sem_post_id(int sem_id) {
    if (sem_id < 0 || sem_id >= SEM_MAX) return -1;
    struct semaphore* s = &sems[sem_id];
    uint32_t flags;
    spin_lock_irqsave(&s->lock, &flags);
    if (!s->valid) { spin_unlock_irqrestore(&s->lock, flags); return -1; }
    s->value++;
    if (s->wait_count > 0) {
        struct task* t = s->wait_queue[0];
        // shift queue
        for (int i = 1; i < s->wait_count; i++) s->wait_queue[i-1] = s->wait_queue[i];
        s->wait_count--;
        t->state = TASK_READY;
    }
    spin_unlock_irqrestore(&s->lock, flags);
    return 0;
}

int sem_destroy_id(int sem_id) {
    if (sem_id < 0 || sem_id >= SEM_MAX) return -1;
    struct semaphore* s = &sems[sem_id];
    uint32_t flags;
    spin_lock_irqsave(&s->lock, &flags);
    if (!s->valid) { spin_unlock_irqrestore(&s->lock, flags); return -1; }
    // wake all waiters
    for (int i = 0; i < s->wait_count; i++) {
        s->wait_queue[i]->state = TASK_READY;
    }
    s->wait_count = 0;
    s->valid = 0;
    spin_unlock_irqrestore(&s->lock, flags);
    return 0;
}
