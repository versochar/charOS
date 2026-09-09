/* 34B: boot services cleanup kaydi. */
#include "cleanup64.h"

static void *cleanup64_tab[CLEANUP64_MAX];
static int cleanup64_n = 0;

int cleanup64_track(void *handle) {
    int i;
    if (!handle) return -3;
    for (i = 0; i < cleanup64_n; i++)
        if (cleanup64_tab[i] == handle) return -2;
    if (cleanup64_n >= CLEANUP64_MAX) return -1;
    cleanup64_tab[cleanup64_n++] = handle;
    return 0;
}

int cleanup64_count(void) { return cleanup64_n; }

void *cleanup64_get(int i) {
    if (i < 0 || i >= cleanup64_n) return 0;
    return cleanup64_tab[i];
}

void cleanup64_untrack(void *handle) {
    int i, j;
    for (i = 0; i < cleanup64_n; i++) {
        if (cleanup64_tab[i] == handle) {
            for (j = i; j + 1 < cleanup64_n; j++)
                cleanup64_tab[j] = cleanup64_tab[j + 1];
            cleanup64_n--;
            return;
        }
    }
}

void cleanup64_clear(void) { cleanup64_n = 0; }
