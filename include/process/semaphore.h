#ifndef CHAROS_PROCESS_SEMAPHORE_H
#define CHAROS_PROCESS_SEMAPHORE_H

#include <stdint.h>
#include <core/spinlock.h>

#define SEM_MAX 16

struct semaphore {
    int valid;
    int value;
    int wait_count;
    struct task* wait_queue[16];
    spinlock_t lock;
};

void sem_init_all(void);
int sem_create(int value);
int sem_wait_id(int sem_id);
int sem_post_id(int sem_id);
int sem_destroy_id(int sem_id);

#endif
