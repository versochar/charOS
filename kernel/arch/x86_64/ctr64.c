#include "arch/x86_64/ctr.h"
#include <string.h>

struct ctr_slot {
    char name[CTR64_NAME_MAX];
    char image[CTR64_IMAGE_MAX];
    u64  mem_limit;
    u64  pid;
    int  id;
    int  state;
    int  used;
};
static struct ctr_slot ctr_tab[CTR64_MAX_CONTAINERS];
static int ctr_next_id = 1;
static u64 ctr_next_pid = 100;

static int ctr_find(int id) {
    int i;
    for (i = 0; i < CTR64_MAX_CONTAINERS; i++)
        if (ctr_tab[i].used && ctr_tab[i].id == id)
            return i;
    return -1;
}

int ctr64_init(void) {
    memset(ctr_tab, 0, sizeof(ctr_tab));
    ctr_next_id = 1;
    ctr_next_pid = 100;
    return 0;
}

int ctr64_create(const char *name, const char *image, u64 mem_limit_bytes,
                 int *out_id) {
    int i;
    if (!name || !image || !out_id) return -1;
    if (mem_limit_bytes == 0) return -2;
    for (i = 0; i < CTR64_MAX_CONTAINERS; i++) {
        if (ctr_tab[i].used && strcmp(ctr_tab[i].name, name) == 0)
            return -3; /* ayni isim */
    }
    for (i = 0; i < CTR64_MAX_CONTAINERS; i++) {
        if (!ctr_tab[i].used) {
            ctr_tab[i].used = 1;
            ctr_tab[i].id = ctr_next_id++;
            ctr_tab[i].pid = ctr_next_pid;
            ctr_next_pid += 100;
            strncpy(ctr_tab[i].name, name, sizeof(ctr_tab[i].name) - 1);
            strncpy(ctr_tab[i].image, image, sizeof(ctr_tab[i].image) - 1);
            ctr_tab[i].mem_limit = mem_limit_bytes;
            ctr_tab[i].state = CTR64_CREATED;
            *out_id = ctr_tab[i].id;
            return 0;
        }
    }
    return -4;
}

int ctr64_start(int id) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (ctr_tab[i].state != CTR64_CREATED) return -2;
    ctr_tab[i].state = CTR64_RUNNING;
    return 0;
}

int ctr64_pause(int id) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (ctr_tab[i].state != CTR64_RUNNING) return -2;
    ctr_tab[i].state = CTR64_PAUSED;
    return 0;
}

int ctr64_resume(int id) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (ctr_tab[i].state != CTR64_PAUSED) return -2;
    ctr_tab[i].state = CTR64_RUNNING;
    return 0;
}

int ctr64_stop(int id) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (ctr_tab[i].state == CTR64_STOPPED) return -2;
    ctr_tab[i].state = CTR64_STOPPED;
    return 0;
}

int ctr64_state(int id, int *out) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (!out) return -1;
    *out = ctr_tab[i].state;
    return 0;
}

int ctr64_count(void) {
    int i, c = 0;
    for (i = 0; i < CTR64_MAX_CONTAINERS; i++)
        if (ctr_tab[i].used) c++;
    return c;
}

int ctr64_pid(int id, u64 *out_pid) {
    int i = ctr_find(id);
    if (i < 0) return -1;
    if (!out_pid) return -1;
    *out_pid = ctr_tab[i].pid;
    return 0;
}