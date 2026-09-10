#include "arch/x86_64/longmode.h"
#include "arch/x86_64/proc.h"

#define PROC_MAX 64
#define PROC_FREE 0
#define PROC_READY 1
#define PROC_RUNNING 2
#define PROC_ZOMBIE 3

static int proc_inited = 0;
static u64 pid_ctr = 0;

typedef struct {
    u64 pid;
    int state;
    int code;
} proc_slot_t;

static proc_slot_t slots[PROC_MAX];

static proc_slot_t *find_slot(u64 pid) {
    for (int i = 0; i < PROC_MAX; i++) {
        if (slots[i].state != PROC_FREE && slots[i].pid == pid) return &slots[i];
    }
    return 0;
}

int proc64_init(void) {
    proc_inited = 1;
    pid_ctr = 0;
    for (int i = 0; i < PROC_MAX; i++) {
        slots[i].pid = 0;
        slots[i].state = PROC_FREE;
        slots[i].code = 0;
    }
    return 0;
}

int proc64_spawn(u64 *out_pid) {
    if (!proc_inited || !out_pid) return -1;
    for (int i = 0; i < PROC_MAX; i++) {
        if (slots[i].state == PROC_FREE) {
            pid_ctr++;
            slots[i].pid = pid_ctr;
            slots[i].state = PROC_READY;
            slots[i].code = 0;
            *out_pid = pid_ctr;
            return 0;
        }
    }
    return -1;
}

int proc64_exit(u64 pid, int code) {
    proc_slot_t *s = find_slot(pid);
    if (!s) return -1;
    if (s->state == PROC_ZOMBIE || s->state == PROC_FREE) return -1;
    s->state = PROC_ZOMBIE;
    s->code = code;
    return 0;
}

int proc64_wait(u64 pid, int *out_code) {
    proc_slot_t *s = find_slot(pid);
    if (!s) return -1;
    if (s->state != PROC_ZOMBIE) return -1;
    if (out_code) *out_code = s->code;
    s->state = PROC_FREE;
    s->pid = 0;
    return 0;
}

int proc64_state(u64 pid, int *out_state) {
    proc_slot_t *s = find_slot(pid);
    if (!s || !out_state) return -1;
    *out_state = s->state;
    return 0;
}
