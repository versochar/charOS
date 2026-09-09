/* 37B: NX zorunlulugu — veri/yiginda X biti her zaman temizlenir. */
#include "arch/x86_64/longmode.h"

u64 nx64_enforce(u64 req_flags, int is_stack_or_heap) {
    u64 f = req_flags & (NX64_R | NX64_W | NX64_X);
    if (!(f & NX64_R)) f |= NX64_R; /* okuma her zaman */
    if (is_stack_or_heap) f &= ~NX64_X; /* veri calistirilamaz */
    return f;
}

int nx64_exec_allowed(u64 flags) {
    return (flags & NX64_X) ? 1 : 0;
}

u64 nx64_stack_flags(void) {
    return NX64_R | NX64_W; /* X yok */
}
