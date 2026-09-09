/* 35I: memtest 64-bit — yuruyen birler, adres-verisi, dama tahtasi.
 * Donus: 0=saglam, >0 ilk hata ofseti+1.
 */
#include "arch/x86_64/longmode.h"

int memtest64_run(void *buf, u64 len) {
    volatile unsigned char *b = (volatile unsigned char *)buf;
    u64 i, j;
    if (!b || !len) return -1;
    /* 1) Sifirla + adres-verisi */
    for (i = 0; i < len; i++) b[i] = (unsigned char)(i & 0xFF);
    for (i = 0; i < len; i++)
        if (b[i] != (unsigned char)(i & 0xFF)) return (int)(i + 1);
    /* 2) Yuruyen birler (ilk 64 baytta bit duzeyi) */
    for (j = 0; j < 8 && j < len; j++) {
        for (i = 0; i < 8; i++) {
            u64 off = j * 8 + i;
            unsigned char pat;
            if (off >= len) break;
            pat = (unsigned char)(1U << i);
            b[off] = pat;
            if (b[off] != pat) return (int)(off + 1);
            b[off] = (unsigned char)~pat;
            if (b[off] != (unsigned char)~pat) return (int)(off + 1);
        }
    }
    /* 3) Dama tahtasi + tersi */
    for (i = 0; i < len; i++) b[i] = (i & 1) ? 0x55 : 0xAA;
    for (i = 0; i < len; i++) {
        unsigned char exp = (i & 1) ? 0x55 : 0xAA;
        if (b[i] != exp) return (int)(i + 1);
        b[i] = (unsigned char)~exp;
    }
    for (i = 0; i < len; i++) {
        unsigned char exp = (i & 1) ? (unsigned char)~0x55 : (unsigned char)~0xAA;
        if (b[i] != exp) return (int)(i + 1);
    }
    return 0;
}
