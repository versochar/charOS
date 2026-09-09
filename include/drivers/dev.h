#ifndef CHAROS_DRIVERS_DEV_H
#define CHAROS_DRIVERS_DEV_H

#include <stdint.h>

/* 14J: birleşik cihaz modeli + devfs.
 * Sürücüler ad/tür/ops ile kaydolur; okuma/yazma/ioctl tek API'den.
 * (chfs mount yok — /dev ad alanı kayıt tablosudur.) */

#define DEV_MAX 16
#define DEV_NAME_LEN 16

enum dev_type {
    DEV_CHR = 1,   /* karakter cihazı (kbd, serial, null, zero) */
    DEV_BLK = 2,   /* blok cihazı (vda) */
    DEV_FB  = 3    /* framebuffer */
};

struct device;
typedef int (*dev_read_fn)(struct device* d, char* buf, int len);
typedef int (*dev_write_fn)(struct device* d, const char* buf, int len);
typedef int (*dev_ioctl_fn)(struct device* d, int cmd, int arg);

struct device {
    int used;
    char name[DEV_NAME_LEN];
    int type;
    dev_read_fn read;
    dev_write_fn write;
    dev_ioctl_fn ioctl;
    void* priv;
};

int dev_init(void);
int dev_register(const char* name, int type, dev_read_fn r, dev_write_fn w,
                 dev_ioctl_fn io);          /* id / -1 dolu|tekrar */
int dev_lookup(const char* name);           /* id / -1 yok */
int dev_read(int id, char* buf, int len);
int dev_write(int id, const char* buf, int len);
int dev_ioctl(int id, int cmd, int arg);
int dev_count(void);
int dev_name(int id, char* out, int max);   /* listeleme */
int dev_selftest(void);                     /* kayıt+null/zero/kbd. 0 PASS. */

#endif
