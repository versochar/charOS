/* 39H: ioctl yonlendirme — (cihaz, komut) -> isleyici tablosu. */
#include "arch/x86_64/longmode.h"

#define IOCTL64_MAX 64

struct ioctl64_entry {
    int used;
    u64 dev;
    u32 cmd;
    ioctl64_fn fn;
};

static struct ioctl64_entry ioctl64_tab[IOCTL64_MAX];

int ioctl64_register(u64 dev, u32 cmd, ioctl64_fn fn) {
    int i;
    if (!fn) return -1;
    for (i = 0; i < IOCTL64_MAX; i++) {
        if (ioctl64_tab[i].used && ioctl64_tab[i].dev == dev &&
            ioctl64_tab[i].cmd == cmd) {
            ioctl64_tab[i].fn = fn;
            return 0;
        }
    }
    for (i = 0; i < IOCTL64_MAX; i++) {
        if (!ioctl64_tab[i].used) {
            ioctl64_tab[i].used = 1;
            ioctl64_tab[i].dev = dev;
            ioctl64_tab[i].cmd = cmd;
            ioctl64_tab[i].fn = fn;
            return 0;
        }
    }
    return -2;
}

int ioctl64_call(u64 dev, u32 cmd, u64 arg) {
    int i;
    for (i = 0; i < IOCTL64_MAX; i++) {
        if (ioctl64_tab[i].used && ioctl64_tab[i].dev == dev &&
            ioctl64_tab[i].cmd == cmd)
            return ioctl64_tab[i].fn(arg);
    }
    return -1; /* kayitsiz */
}
