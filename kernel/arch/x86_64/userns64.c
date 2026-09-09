/* 37H / 58A: user namespaces */
#include "arch/x86_64/longmode.h"

static int ns_level = 0;
static int map_ext_start = 0;
static int map_int_start = 0;
static int map_count = 0;

int userns64_add_map(u32 ext_start, u32 int_start, u32 count) {
    if (count <= 0) return -1;
    map_ext_start = ext_start;
    map_int_start = int_start;
    map_count = count;
    return 0;
}

int userns64_map_uid(u32 ext_uid, u32 *out) {
    if (!out) return -1;
    if (ext_uid < map_ext_start || ext_uid >= map_ext_start + map_count) return -1;
    *out = map_int_start + (ext_uid - map_ext_start);
    return 0;
}

int userns64_level(void) {
    return ns_level;
}

void userns64_enter(void) {
    ns_level++;
}
