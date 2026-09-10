#include "arch/x86_64/longmode.h"
#include "arch/x86_64/part.h"

#define PART_MAX 16

typedef struct {
    u64 part;
    u64 dev;
    u64 start;
    u64 len;
    int type;
    int active;
} part_slot_t;

static int part_inited = 0;
static u64 part_ctr = 0;
static part_slot_t parts[PART_MAX];

static part_slot_t *find_part(u64 part) {
    for (int i = 0; i < PART_MAX; i++) {
        if (parts[i].active && parts[i].part == part) return &parts[i];
    }
    return 0;
}

static int overlaps(u64 dev, u64 s, u64 l) {
    for (int i = 0; i < PART_MAX; i++) {
        if (!parts[i].active || parts[i].dev != dev) continue;
        u64 a = parts[i].start, b = parts[i].start + parts[i].len;
        u64 c = s, d = s + l;
        if (c < b && a < d) return 1;
    }
    return 0;
}

int part64_init(void) {
    part_inited = 1;
    part_ctr = 0;
    for (int i = 0; i < PART_MAX; i++) {
        parts[i].part = 0; parts[i].active = 0;
    }
    return 0;
}

int part64_add(u64 dev, u64 start, u64 len, int type, u64 *out_part) {
    if (!part_inited || !out_part || len == 0 || type < 0 || type > 3) return -1;
    if (overlaps(dev, start, len)) return -2;
    for (int i = 0; i < PART_MAX; i++) {
        if (!parts[i].active) {
            part_ctr++;
            parts[i].part = part_ctr;
            parts[i].dev = dev;
            parts[i].start = start;
            parts[i].len = len;
            parts[i].type = type;
            parts[i].active = 1;
            *out_part = part_ctr;
            return 0;
        }
    }
    return -1;
}

int part64_del(u64 part) {
    part_slot_t *p = find_part(part);
    if (!p) return -1;
    p->active = 0; p->part = 0;
    return 0;
}

int part64_info(u64 part, u64 *out_dev, u64 *out_start, u64 *out_len) {
    part_slot_t *p = find_part(part);
    if (!p) return -1;
    if (out_dev) *out_dev = p->dev;
    if (out_start) *out_start = p->start;
    if (out_len) *out_len = p->len;
    return 0;
}
