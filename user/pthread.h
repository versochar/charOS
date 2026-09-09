#ifndef USER_PTHREAD_H
#define USER_PTHREAD_H

/* 15B: POSIX thread API-lite (futex tabanlı senkronizasyon).
 * pthread_t = kernel tid (waitpid ile join). Mutex: 3-durumlu futex
 * kilidi (0 açık, 1 kilitli, 2 çekişmeli). Condvar: sıra sayaçlı. */

typedef int pthread_t;
typedef void* (*pthread_fn)(void*);

typedef struct { volatile int state; } pthread_mutex_t;
typedef struct { volatile unsigned seq; } pthread_cond_t;

#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_COND_INITIALIZER {0}

int pthread_create(pthread_t* th, void* attr, pthread_fn fn, void* arg);
int pthread_join(pthread_t th, void** retval);
int pthread_mutex_init(pthread_mutex_t* m, void* attr);
int pthread_mutex_lock(pthread_mutex_t* m);
int pthread_mutex_unlock(pthread_mutex_t* m);
int pthread_cond_init(pthread_cond_t* c, void* attr);
int pthread_cond_wait(pthread_cond_t* c, pthread_mutex_t* m);
int pthread_cond_signal(pthread_cond_t* c);
int pthread_cond_broadcast(pthread_cond_t* c);

#endif
