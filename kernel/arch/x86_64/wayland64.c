/* 55A: minimal Wayland compositor — global/nesne/olay dagitimi. */
#include "arch/x86_64/longmode.h"

#define WAYLAND64_MAX_GLOBAL 16
#define WAYLAND64_MAX_OBJ 64
#define WAYLAND64_MAX_METHOD 16
#define WAYLAND64_MAX_EV 64

struct wayland64_global {
    int used;
    char interface[32];
    u32 version;
    u32 next_id;
};

struct wayland64_method {
    u32 obj;
    char interface[32];
    u16 opcode;
    wayland64_fn fn;
};

struct wayland64_obj {
    int used;
    u32 id;
    char interface[32];
};

struct wayland64_ev {
    u32 obj;
    u16 opcode;
    u64 arg;
};

static struct wayland64_global wayland64_globals[WAYLAND64_MAX_GLOBAL];
static struct wayland64_obj wayland64_objs[WAYLAND64_MAX_OBJ];
static struct wayland64_method wayland64_methods[WAYLAND64_MAX_METHOD];
static struct wayland64_ev wayland64_q[WAYLAND64_MAX_EV];
static int wayland64_head = 0;
static int wayland64_n = 0;
static u32 wayland64_next_id = 1;

static void wl_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int wl_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int wayland64_global_add(const char *interface, u32 version) {
    int i;
    if (!interface) return -1;
    for (i = 0; i < WAYLAND64_MAX_GLOBAL; i++) {
        if (!wayland64_globals[i].used) {
            wayland64_globals[i].used = 1;
            wl_str_copy(wayland64_globals[i].interface, interface, 32);
            wayland64_globals[i].version = version;
            wayland64_globals[i].next_id = 0;
            return 0;
        }
    }
    return -2;
}

int wayland64_bind(const char *interface, u32 version, u32 *id_out) {
    int i, g = -1;
    for (i = 0; i < WAYLAND64_MAX_GLOBAL; i++) {
        if (wayland64_globals[i].used &&
            wl_str_eq(wayland64_globals[i].interface, interface) &&
            wayland64_globals[i].version >= version) {
            g = i;
            break;
        }
    }
    if (g < 0) return -1;
    for (i = 0; i < WAYLAND64_MAX_OBJ; i++) {
        if (!wayland64_objs[i].used) {
            wayland64_objs[i].used = 1;
            wayland64_objs[i].id = wayland64_next_id++;
            wl_str_copy(wayland64_objs[i].interface, interface, 32);
            if (id_out) *id_out = wayland64_objs[i].id;
            return 0;
        }
    }
    return -2;
}

int wayland64_method(u32 obj, const char *interface, u16 opcode,
                     wayland64_fn fn) {
    int i;
    if (!interface || !fn) return -1;
    for (i = 0; i < WAYLAND64_MAX_METHOD; i++) {
        if (!wayland64_methods[i].fn) {
            wayland64_methods[i].obj = obj;
            wl_str_copy(wayland64_methods[i].interface, interface, 32);
            wayland64_methods[i].opcode = opcode;
            wayland64_methods[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

static struct wayland64_obj *wayland64_find_obj(u32 id) {
    int i;
    for (i = 0; i < WAYLAND64_MAX_OBJ; i++) {
        if (wayland64_objs[i].used && wayland64_objs[i].id == id)
            return &wayland64_objs[i];
    }
    return 0;
}

int wayland64_dispatch(u32 obj, u16 opcode, u64 arg) {
    struct wayland64_obj *o = wayland64_find_obj(obj);
    int i;
    if (!o) return -1;
    for (i = 0; i < WAYLAND64_MAX_METHOD; i++) {
        if (!wayland64_methods[i].fn) continue;
        if (wayland64_methods[i].obj != obj) continue;
        if (wayland64_methods[i].opcode != opcode) continue;
        if (!wl_str_eq(wayland64_methods[i].interface, o->interface))
            continue;
        return wayland64_methods[i].fn(obj, opcode, arg);
    }
    return -2; /* yontem yok */
}

int wayland64_event(u32 obj, u16 opcode, u64 arg) {
    if (wayland64_n >= WAYLAND64_MAX_EV) return -1;
    wayland64_q[(wayland64_head + wayland64_n) % WAYLAND64_MAX_EV].obj =
        obj;
    wayland64_q[(wayland64_head + wayland64_n) % WAYLAND64_MAX_EV].opcode =
        opcode;
    wayland64_q[(wayland64_head + wayland64_n) % WAYLAND64_MAX_EV].arg =
        arg;
    wayland64_n++;
    return 0;
}

int wayland64_poll(u32 *obj, u16 *opcode, u64 *arg) {
    if (!wayland64_n) return -1;
    if (obj) *obj = wayland64_q[wayland64_head].obj;
    if (opcode) *opcode = wayland64_q[wayland64_head].opcode;
    if (arg) *arg = wayland64_q[wayland64_head].arg;
    wayland64_head = (wayland64_head + 1) % WAYLAND64_MAX_EV;
    wayland64_n--;
    return 0;
}
