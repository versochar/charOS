/* 45I: USB devfs — cihaz dugumleri + tanimlayici deposu. */
#include "arch/x86_64/longmode.h"

#define USBDEVFS64_MAX 32
#define USBDEVFS64_DESC 256

struct usbdevfs64_dev {
    int used;
    int bus;
    int addr;
    u16 vid;
    u16 pid;
    int speed;
    unsigned char desc[USBDEVFS64_DESC];
    int desclen;
};

static struct usbdevfs64_dev usbdevfs64_tab[USBDEVFS64_MAX];

int usbdevfs64_add(int bus, int addr, u16 vid, u16 pid, int speed) {
    int i;
    if (bus < 0 || addr <= 0) return -1;
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        if (usbdevfs64_tab[i].used && usbdevfs64_tab[i].bus == bus &&
            usbdevfs64_tab[i].addr == addr) {
            usbdevfs64_tab[i].vid = vid;
            usbdevfs64_tab[i].pid = pid;
            usbdevfs64_tab[i].speed = speed;
            return 0;
        }
    }
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        if (!usbdevfs64_tab[i].used) {
            usbdevfs64_tab[i].used = 1;
            usbdevfs64_tab[i].bus = bus;
            usbdevfs64_tab[i].addr = addr;
            usbdevfs64_tab[i].vid = vid;
            usbdevfs64_tab[i].pid = pid;
            usbdevfs64_tab[i].speed = speed;
            usbdevfs64_tab[i].desclen = 0;
            return 0;
        }
    }
    return -2;
}

int usbdevfs64_remove(int bus, int addr) {
    int i;
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        if (usbdevfs64_tab[i].used && usbdevfs64_tab[i].bus == bus &&
            usbdevfs64_tab[i].addr == addr) {
            usbdevfs64_tab[i].used = 0;
            return 0;
        }
    }
    return -1;
}

int usbdevfs64_find(u16 vid, u16 pid, int *bus, int *addr) {
    int i;
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        if (!usbdevfs64_tab[i].used) continue;
        if (usbdevfs64_tab[i].vid != vid) continue;
        if (pid && usbdevfs64_tab[i].pid != pid) continue;
        if (bus) *bus = usbdevfs64_tab[i].bus;
        if (addr) *addr = usbdevfs64_tab[i].addr;
        return 0;
    }
    return -1;
}

int usbdevfs64_desc(int bus, int addr, const unsigned char *desc, int len) {
    int i, k;
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        if (!usbdevfs64_tab[i].used || usbdevfs64_tab[i].bus != bus ||
            usbdevfs64_tab[i].addr != addr)
            continue;
        if (!desc || len <= 0 || len > USBDEVFS64_DESC) return -1;
        for (k = 0; k < len; k++) usbdevfs64_tab[i].desc[k] = desc[k];
        usbdevfs64_tab[i].desclen = len;
        return 0;
    }
    return -2;
}

int usbdevfs64_read_desc(int bus, int addr, unsigned char *out, int max) {
    int i, n;
    for (i = 0; i < USBDEVFS64_MAX; i++) {
        int k;
        if (!usbdevfs64_tab[i].used || usbdevfs64_tab[i].bus != bus ||
            usbdevfs64_tab[i].addr != addr)
            continue;
        if (!out || max <= 0) return -1;
        n = usbdevfs64_tab[i].desclen < max ? usbdevfs64_tab[i].desclen
                                            : max;
        for (k = 0; k < n; k++) out[k] = usbdevfs64_tab[i].desc[k];
        return n;
    }
    return -2;
}
