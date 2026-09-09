/* 51B: pthread — gorev tablosu + mutex + cond (tek-cekirdek semantigi).
 * Gercek on-alimli zamanlama 33x'e baglanir; API ve durumlar simdiden sabit.
 */
#include "arch/x86_64/longmode.h"

#define PTHREAD64_MAX 16
#define PTHREAD64_MAX_MUTEX 16
#define PTHREAD64_MAX_COND 16

#define PTHREAD64_EXITED 3

struct pthread64_tcb {
    int used;
    int state; /* INIT64_* yeniden kullanilir: 0 bos degil... */
    int detached;
    int retval;
};

struct pthread64_mutex {
    int used;
    int locked;
    int owner;
    int count;
};

struct pthread64_cond {
    int used;
    int waiters;
};

static struct pthread64_tcb pthread64_tab[PTHREAD64_MAX];
static struct pthread64_mutex pthread64_mtx[PTHREAD64_MAX_MUTEX];
static struct pthread64_cond pthread64_cond[PTHREAD64_MAX_COND];
static int pthread64_self_id = 0;

int pthread64_create(int *id_out) {
    int i;
    for (i = 0; i < PTHREAD64_MAX; i++) {
        if (!pthread64_tab[i].used) {
            pthread64_tab[i].used = 1;
            pthread64_tab[i].state = INIT64_STARTING;
            pthread64_tab[i].detached = 0;
            pthread64_tab[i].retval = 0;
            if (id_out) *id_out = i;
            return 0;
        }
    }
    return -1;
}

int pthread64_join(int id, int *retval) {
    if (id < 0 || id >= PTHREAD64_MAX || !pthread64_tab[id].used)
        return -1;
    if (pthread64_tab[id].detached) return -2;
    /* Skeleton: gorev hemen bitmis sayilir */
    pthread64_tab[id].state = PTHREAD64_EXITED;
    if (retval) *retval = pthread64_tab[id].retval;
    pthread64_tab[id].used = 0;
    return 0;
}

int pthread64_detach(int id) {
    if (id < 0 || id >= PTHREAD64_MAX || !pthread64_tab[id].used)
        return -1;
    pthread64_tab[id].detached = 1;
    return 0;
}

int pthread64_self(void) { return pthread64_self_id; }

int pthread64_mutex_init(void) {
    int i;
    for (i = 0; i < PTHREAD64_MAX_MUTEX; i++) {
        if (!pthread64_mtx[i].used) {
            pthread64_mtx[i].used = 1;
            pthread64_mtx[i].locked = 0;
            pthread64_mtx[i].owner = -1;
            pthread64_mtx[i].count = 0;
            return i;
        }
    }
    return -1;
}

static struct pthread64_mutex *pthread64_mget(int m) {
    if (m < 0 || m >= PTHREAD64_MAX_MUTEX || !pthread64_mtx[m].used)
        return 0;
    return &pthread64_mtx[m];
}

int pthread64_mutex_lock(int m) {
    struct pthread64_mutex *x = pthread64_mget(m);
    if (!x) return -1;
    if (x->locked && x->owner == pthread64_self_id) {
        x->count++; /* ozyinelemeli */
        return 0;
    }
    if (x->locked) return -2; /* bloklama yok (skeleton) */
    x->locked = 1;
    x->owner = pthread64_self_id;
    x->count = 1;
    return 0;
}

int pthread64_mutex_trylock(int m) { return pthread64_mutex_lock(m); }

int pthread64_mutex_unlock(int m) {
    struct pthread64_mutex *x = pthread64_mget(m);
    if (!x || !x->locked || x->owner != pthread64_self_id) return -1;
    if (--x->count <= 0) {
        x->count = 0;
        x->locked = 0;
        x->owner = -1;
    }
    return 0;
}

int pthread64_cond_wait(int c, int m) {
    struct pthread64_mutex *x = pthread64_mget(m);
    if (c < 0 || c >= PTHREAD64_MAX_COND || !x) return -1;
    pthread64_cond[c].used = 1;
    pthread64_cond[c].waiters++;
    pthread64_mutex_unlock(m);
    /* Skeleton: sinyal hemen gelmis sayilir */
    if (pthread64_cond[c].waiters > 0) pthread64_cond[c].waiters--;
    return pthread64_mutex_lock(m);
}

int pthread64_cond_signal(int c) {
    if (c < 0 || c >= PTHREAD64_MAX_COND) return -1;
    if (pthread64_cond[c].waiters > 0) pthread64_cond[c].waiters--;
    return 0;
}

int pthread64_cond_broadcast(int c) {
    if (c < 0 || c >= PTHREAD64_MAX_COND) return -1;
    pthread64_cond[c].waiters = 0;
    return 0;
}
