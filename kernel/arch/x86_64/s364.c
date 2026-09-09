/* 48B: S3 — cihaz askıya alma sirasi + durum makinesi. */
#include "arch/x86_64/longmode.h"

#define S364_MAX_DEV 16

struct s364_dev {
    int used;
    char name[32];
    int (*suspend)(void);
    int (*resume)(void);
};

static struct s364_dev s364_tab[S364_MAX_DEV];
static int s364_st = 0; /* 0=acik,1=askida,2=uyaniyor */

static void s3_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int s364_register(const char *dev, int (*suspend)(void),
                  int (*resume)(void)) {
    int i;
    if (!dev) return -1;
    for (i = 0; i < S364_MAX_DEV; i++) {
        if (!s364_tab[i].used) {
            s364_tab[i].used = 1;
            s3_str_copy(s364_tab[i].name, dev, 32);
            s364_tab[i].suspend = suspend;
            s364_tab[i].resume = resume;
            return 0;
        }
    }
    return -2;
}

int s364_suspend(u8 typ_a, u8 typ_b) {
    int i;
    (void)typ_a;
    (void)typ_b;
    if (s364_st != 0) return -1;
    /* Cihazlar kayit sirasinda askıya alinir (gercek PM yazmaci 50x'te) */
    for (i = 0; i < S364_MAX_DEV; i++) {
        if (!s364_tab[i].used || !s364_tab[i].suspend) continue;
        if (s364_tab[i].suspend() != 0) {
            /* Hata: o ana kadarkileri geri uyandir */
            int j;
            for (j = i - 1; j >= 0; j--) {
                if (s364_tab[j].used && s364_tab[j].resume)
                    s364_tab[j].resume();
            }
            return -2;
        }
    }
    s364_st = 1;
    return 0;
}

int s364_resume(void) {
    int i;
    if (s364_st != 1) return -1;
    s364_st = 2;
    for (i = S364_MAX_DEV - 1; i >= 0; i--) {
        if (!s364_tab[i].used || !s364_tab[i].resume) continue;
        if (s364_tab[i].resume() != 0) {
            s364_st = 1;
            return -2;
        }
    }
    s364_st = 0;
    return 0;
}

int s364_state(void) { return s364_st; }
