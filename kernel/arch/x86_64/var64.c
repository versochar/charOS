/* 34I: firmware degiskeni — ad/boyut/GUID dogrulama (saf mantik). */
#include "arch/x86_64/longmode.h"

int var64_name_ok(const unsigned short *name) {
    int len = 0;
    if (!name) return -1;
    while (len < 65) {
        unsigned short c = name[len];
        if (!c) break;
        if (c < 0x20 || c > 0x7E) return -2; /* yazdirilabilir ASCII */
        len++;
    }
    if (!len) return -3;   /* bos ad */
    if (len > 64) return -4;
    return 0;
}

int var64_size_ok(u64 size, u64 max) {
    if (!max) return -1;
    if (!size) return -2;
    if (size > max) return -3;
    return 0;
}

int var64_guid_eq(const unsigned char a[16], const unsigned char b[16]) {
    int i;
    if (!a || !b) return -1;
    for (i = 0; i < 16; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}
