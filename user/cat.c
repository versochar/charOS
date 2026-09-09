/* 19C: /bin/cat — stdin'i stdout'a kopyala (EOF'a kadar). */
typedef unsigned int u32;
#define SYS_READ 2
#define SYS_WRITE 1
static inline u32 sc(u32 n, u32 a, u32 b, u32 c) {
    u32 r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory"); return r;
}
int umain(int argc, char** argv) {
    (void)argc; (void)argv;
    static char buf[256];
    for (;;) {
        int r = (int)sc(SYS_READ, 0, (u32)buf, sizeof(buf));
        if (r <= 0) break;
        u32 off = 0;
        while (off < (u32)r) {
            int w = (int)sc(SYS_WRITE, 1, (u32)(buf + off), (u32)r - off);
            if (w <= 0) break;
            off += (u32)w;
        }
        if (off < (u32)r) break;
    }
    return 0;
}
#include "args.h"
