#include <drivers/dev.h>
#include <drivers/keyboard.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

/* 14J: cihaz kayıt tablosu + yerleşik null/zero/kbd. */

static struct device devs[DEV_MAX];
static int dev_on = 0;

static int dev_null_read(struct device* d, char* buf, int len) {
    (void)d; (void)buf; (void)len;
    return 0; /* EOF */
}

static int dev_null_write(struct device* d, const char* buf, int len) {
    (void)d; (void)buf;
    return len < 0 ? -1 : len; /* çöpe at */
}

static int dev_zero_read(struct device* d, char* buf, int len) {
    (void)d;
    if (!buf || len < 0) return -1;
    for (int i = 0; i < len; i++) buf[i] = 0;
    return len;
}

static int dev_kbd_read(struct device* d, char* buf, int len) {
    (void)d;
    if (!buf || len <= 0) return -1;
    char c = keyboard_getchar_nonblock();
    if (!c) return 0; /* bloklanmaz */
    buf[0] = c;
    return 1;
}

int dev_init(void) {
    if (dev_on) return 0;
    memset(devs, 0, sizeof(devs));
    dev_on = 1;
    dev_register("null", DEV_CHR, dev_null_read, dev_null_write, 0);
    dev_register("zero", DEV_CHR, dev_zero_read, dev_null_write, 0);
    dev_register("kbd", DEV_CHR, dev_kbd_read, 0, 0);
    serial_puts("[14J] devfs hazır (null, zero, kbd)\n");
    return 0;
}

int dev_register(const char* name, int type, dev_read_fn r, dev_write_fn w,
                 dev_ioctl_fn io) {
    if (!dev_on || !name || !name[0]) return -1;
    for (int i = 0; i < DEV_MAX; i++)
        if (devs[i].used && strcmp(devs[i].name, name) == 0) return -1;
    for (int i = 0; i < DEV_MAX; i++) {
        if (devs[i].used) continue;
        devs[i].used = 1;
        strncpy(devs[i].name, name, DEV_NAME_LEN - 1);
        devs[i].name[DEV_NAME_LEN - 1] = 0;
        devs[i].type = type;
        devs[i].read = r;
        devs[i].write = w;
        devs[i].ioctl = io;
        devs[i].priv = 0;
        return i;
    }
    return -1;
}

int dev_lookup(const char* name) {
    if (!dev_on || !name) return -1;
    for (int i = 0; i < DEV_MAX; i++)
        if (devs[i].used && strcmp(devs[i].name, name) == 0) return i;
    return -1;
}

int dev_read(int id, char* buf, int len) {
    if (id < 0 || id >= DEV_MAX || !devs[id].used) return -1;
    if (!devs[id].read) return -1;
    return devs[id].read(&devs[id], buf, len);
}

int dev_write(int id, const char* buf, int len) {
    if (id < 0 || id >= DEV_MAX || !devs[id].used) return -1;
    if (!devs[id].write) return -1;
    return devs[id].write(&devs[id], buf, len);
}

int dev_ioctl(int id, int cmd, int arg) {
    if (id < 0 || id >= DEV_MAX || !devs[id].used) return -1;
    if (!devs[id].ioctl) return -1;
    return devs[id].ioctl(&devs[id], cmd, arg);
}

int dev_count(void) {
    int n = 0;
    for (int i = 0; i < DEV_MAX; i++)
        if (devs[i].used) n++;
    return n;
}

int dev_name(int id, char* out, int max) {
    if (id < 0 || id >= DEV_MAX || !devs[id].used) return -1;
    if (!out || max <= 0) return -1;
    int i = 0;
    while (i < max - 1 && devs[id].name[i]) {
        out[i] = devs[id].name[i];
        i++;
    }
    out[i] = 0;
    return 0;
}

static int dev_t_write(struct device* d, const char* buf, int len) {
    (void)d; (void)buf;
    return len;
}

int dev_selftest(void) {
    int ok = 1;
    if (!dev_on && dev_init() != 0) return -1;
    /* Yerleşikler kayıtlı olmalı */
    int zn = dev_lookup("zero");
    int nn = dev_lookup("null");
    int kb = dev_lookup("kbd");
    if (zn < 0 || nn < 0 || kb < 0) ok = 0;
    if (dev_lookup("yok-boyle-cihaz") != -1) ok = 0;
    if (dev_count() < 3) ok = 0;
    /* zero: sıfır dolu */
    {
        char b[16];
        if (dev_read(zn, b, sizeof(b)) != 16) ok = 0;
        else for (int i = 0; i < 16; i++) if (b[i] != 0) ok = 0;
        if (dev_read(zn, 0, 4) != -1) ok = 0;
    }
    /* null: okuma EOF, yazma yutar */
    {
        char b[4] = {1, 2, 3, 4};
        if (dev_read(nn, b, sizeof(b)) != 0) ok = 0;
        if (dev_write(nn, b, sizeof(b)) != 4) ok = 0;
    }
    /* kbd: boşken bloklanmaz (headless'ta 0) */
    {
        char c = 0;
        int r = dev_read(kb, &c, 1);
        if (r != 0 && r != 1) ok = 0;
    }
    /* Özel cihaz kaydı + tekrar reddi + doluluk */
    {
        int t = dev_register("tdev", DEV_CHR, 0, dev_t_write, 0);
        if (t < 0) ok = 0;
        else {
            if (dev_register("tdev", DEV_CHR, 0, dev_t_write, 0) != -1) ok = 0;
            if (dev_write(t, "ab", 2) != 2) ok = 0;
            if (dev_read(t, (char[2]){0}, 2) != -1) ok = 0; /* read yok */
        }
        char nm[DEV_NAME_LEN];
        if (dev_name(zn, nm, sizeof(nm)) != 0) ok = 0;
        else if (strcmp(nm, "zero") != 0) ok = 0;
        if (dev_name(99, nm, sizeof(nm)) != -1) ok = 0;
        if (dev_read(99, nm, 2) != -1) ok = 0;
    }
    if (ok) {
        serial_puts("[14J] devfs null/zero/kbd/register [PASS]\n");
        vga_puts("[14J] devfs null/zero/kbd/register [PASS]\n");
        return 0;
    }
    serial_puts("[14J] devfs [FAIL]\n");
    vga_puts("[14J] devfs [FAIL]\n");
    return -1;
}
