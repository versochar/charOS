/* 42G: timer units — periyodik tetikleme, vadesi gelen servisler. */
#include "arch/x86_64/longmode.h"

#define TIMER64_MAX 16

struct timer64_entry {
    int used;
    char name[32];
    char service[32];
    u64 interval;
    u64 next;
};

static struct timer64_entry timer64_tab[TIMER64_MAX];

static void timer_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int timer64_add(const char *name, const char *service, u64 interval) {
    int i;
    if (!name || !service || !interval) return -1;
    for (i = 0; i < TIMER64_MAX; i++) {
        if (!timer64_tab[i].used) {
            timer64_tab[i].used = 1;
            timer_str_copy(timer64_tab[i].name, name, 32);
            timer_str_copy(timer64_tab[i].service, service, 32);
            timer64_tab[i].interval = interval;
            timer64_tab[i].next = interval; /* ilk ates */
            return 0;
        }
    }
    return -2;
}

/* Vadesi gelenlerin servis adlarini doldurur; ates adedi. */
int timer64_tick(u64 now, char fired[][32], int max) {
    int i, n = 0;
    for (i = 0; i < TIMER64_MAX && n < max; i++) {
        int j;
        if (!timer64_tab[i].used) continue;
        if (now < timer64_tab[i].next) continue;
        for (j = 0; timer64_tab[i].service[j] && j < 31; j++)
            fired[n][j] = timer64_tab[i].service[j];
        fired[n][j] = 0;
        n++;
        timer64_tab[i].next += timer64_tab[i].interval;
        if (timer64_tab[i].next <= now) /* cok kacirdiysa yakala */
            timer64_tab[i].next = now + timer64_tab[i].interval;
    }
    return n;
}

u64 timer64_next_in(u64 now) {
    int i, found = 0;
    u64 best = 0;
    for (i = 0; i < TIMER64_MAX; i++) {
        u64 left;
        if (!timer64_tab[i].used) continue;
        if (timer64_tab[i].next <= now) return 0;
        left = timer64_tab[i].next - now;
        if (!found || left < best) {
            best = left;
            found = 1;
        }
    }
    return found ? best : 0;
}
