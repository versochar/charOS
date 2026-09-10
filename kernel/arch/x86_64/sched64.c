#include "arch/x86_64/longmode.h"
#include "arch/x86_64/sched.h"

#define SCHED_MAX 64

typedef struct {
    u64 pid;
    int prio;
    u64 seq;
    int active;
} sched_slot_t;

static int sched_inited = 0;
static sched_slot_t tasks[SCHED_MAX];
static u64 seq_ctr = 0;

static sched_slot_t *find_task(u64 pid) {
    for (int i = 0; i < SCHED_MAX; i++) {
        if (tasks[i].active && tasks[i].pid == pid) return &tasks[i];
    }
    return 0;
}

static sched_slot_t *pick_next(void) {
    sched_slot_t *best = 0;
    for (int i = 0; i < SCHED_MAX; i++) {
        if (!tasks[i].active) continue;
        if (!best) { best = &tasks[i]; continue; }
        if (tasks[i].prio < best->prio) best = &tasks[i];
        else if (tasks[i].prio == best->prio && tasks[i].seq < best->seq) best = &tasks[i];
    }
    return best;
}

int sched64_init(void) {
    sched_inited = 1;
    seq_ctr = 0;
    for (int i = 0; i < SCHED_MAX; i++) {
        tasks[i].pid = 0;
        tasks[i].prio = 31;
        tasks[i].seq = 0;
        tasks[i].active = 0;
    }
    return 0;
}

int sched64_add(u64 pid, int prio) {
    if (!sched_inited || pid == 0 || prio < 0 || prio > 31) return -1;
    if (find_task(pid)) return -1;
    for (int i = 0; i < SCHED_MAX; i++) {
        if (!tasks[i].active) {
            tasks[i].pid = pid;
            tasks[i].prio = prio;
            tasks[i].seq = seq_ctr++;
            tasks[i].active = 1;
            return 0;
        }
    }
    return -1;
}

int sched64_remove(u64 pid) {
    sched_slot_t *s = find_task(pid);
    if (!s) return -1;
    s->active = 0;
    s->pid = 0;
    return 0;
}

int sched64_set_prio(u64 pid, int prio) {
    sched_slot_t *s = find_task(pid);
    if (!s || prio < 0 || prio > 31) return -1;
    s->prio = prio;
    return 0;
}

int sched64_current(u64 *out_pid) {
    sched_slot_t *b = pick_next();
    if (!b || !out_pid) return -1;
    *out_pid = b->pid;
    return 0;
}

static int rotate_best_class(void) {
    sched_slot_t *b = pick_next();
    if (!b) return -1;
    int p = b->prio;
    u64 min_seq = (u64)-1;
    for (int i = 0; i < SCHED_MAX; i++) {
        if (tasks[i].active && tasks[i].prio == p && tasks[i].seq < min_seq) min_seq = tasks[i].seq;
    }
    for (int i = 0; i < SCHED_MAX; i++) {
        if (tasks[i].active && tasks[i].prio == p && tasks[i].seq == min_seq) {
            tasks[i].seq = seq_ctr++;
            return 0;
        }
    }
    return -1;
}

int sched64_yield(void) {
    if (!sched_inited) return -1;
    return rotate_best_class();
}

int sched64_tick(void) {
    if (!sched_inited) return -1;
    return rotate_best_class();
}
