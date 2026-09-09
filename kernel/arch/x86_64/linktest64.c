/* 53I: link test — tanimsiz sembol sayimi + yerlesim saglamasi. */
#include "arch/x86_64/longmode.h"

/* delayload tablosundaki cozunmemisler = tanimsiz semboller. */
int linktest64_undef(const char **syms, int n) {
    int i, missing = 0;
    if (!syms || n <= 0) return -1;
    for (i = 0; i < n; i++) {
        if (!syms[i]) continue;
        if (!delayload64_call(syms[i])) missing++;
    }
    return missing;
}

/* Yerlesim tabani sayfa-hizali ve sifir-degil mi? */
int linktest64_reloc(u64 base) {
    if (!base) return -1;
    if (base & 0xFFFULL) return -2;
    return 0;
}
