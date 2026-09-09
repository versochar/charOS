/* 49G: SMART — oznitelik tablosu + esik denetimi + saglik. */
#include "arch/x86_64/longmode.h"

#define SMART64_MAX 32

struct smart64_attr {
    int used;
    int id;
    int value;
    int worst;
    int thresh;
    u64 raw;
};

static struct smart64_attr smart64_tab[SMART64_MAX];

int smart64_add(int id, int value, int worst, int thresh, u64 raw) {
    int i;
    if (id <= 0) return -1;
    for (i = 0; i < SMART64_MAX; i++) {
        if (smart64_tab[i].used && smart64_tab[i].id == id) {
            smart64_tab[i].value = value;
            smart64_tab[i].worst = worst;
            smart64_tab[i].thresh = thresh;
            smart64_tab[i].raw = raw;
            return 0;
        }
    }
    for (i = 0; i < SMART64_MAX; i++) {
        if (!smart64_tab[i].used) {
            smart64_tab[i].used = 1;
            smart64_tab[i].id = id;
            smart64_tab[i].value = value;
            smart64_tab[i].worst = worst;
            smart64_tab[i].thresh = thresh;
            smart64_tab[i].raw = raw;
            return 0;
        }
    }
    return -2;
}

/* Esik alti oznitelik sayisi (0=saglam yon). */
int smart64_failing(void) {
    int i, n = 0;
    for (i = 0; i < SMART64_MAX; i++) {
        if (!smart64_tab[i].used) continue;
        if (smart64_tab[i].thresh && smart64_tab[i].value <=
                                         smart64_tab[i].thresh)
            n++;
    }
    return n;
}

int smart64_health(void) { return smart64_failing() ? 0 : 1; }

int smart64_temp(void) {
    int i;
    for (i = 0; i < SMART64_MAX; i++) {
        if (smart64_tab[i].used && smart64_tab[i].id == 194)
            return (int)(smart64_tab[i].raw & 0xFF);
    }
    return -1000;
}
