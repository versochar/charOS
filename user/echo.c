/* 19A: /bin/echo — argv'yi tek satır yaz (19A konvansiyonu). */
typedef unsigned int u32;
#define SYS_WRITE 1
static inline u32 sc(u32 n, u32 a, u32 b, u32 c) {
    u32 r; asm volatile("int $0x80" : "=a"(r) : "a"(n), "b"(a), "c"(b), "d"(c) : "memory"); return r;
}
static void wout(const char* s, u32 n) {
    u32 off = 0;
    while (off < n) {
        int w = (int)sc(SYS_WRITE, 1, (u32)(s + off), n - off);
        if (w <= 0) break;
        off += (u32)w;
    }
}
int umain(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (i > 1) wout(" ", 1);
        const char* s = argv[i];
        u32 n = 0;
        while (s[n]) n++;
        if (n) wout(s, n);
    }
    wout("\n", 1);
    return 0;
}
#include "args.h"
