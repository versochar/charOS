/* 48C: S4 — hazirda-beklet imaji: sayfa yaz + saglama + dogrulama. */
#include "arch/x86_64/longmode.h"

#define S464_MAX_PAGES 64
#define S464_PAGESZ 4096

static unsigned char s464_pages[S464_MAX_PAGES][S464_PAGESZ];
static int s464_used[S464_MAX_PAGES];
static u64 s464_np = 0;
static u64 s464_sum = 0;
static int s464_open = 0;
static int s464_committed = 0;

int s464_begin(u64 pages) {
    u64 i;
    if (s464_open || !pages || pages > S464_MAX_PAGES) return -1;
    for (i = 0; i < S464_MAX_PAGES; i++) s464_used[i] = 0;
    s464_np = pages;
    s464_sum = 0;
    s464_open = 1;
    s464_committed = 0;
    return 0;
}

int s464_write_page(u64 idx, const void *data) {
    const unsigned char *s;
    u64 i;
    if (!s464_open || !data || idx >= s464_np) return -1;
    s = (const unsigned char *)data;
    for (i = 0; i < S464_PAGESZ; i++) {
        s464_pages[idx][i] = s[i];
        s464_sum += s[i];
    }
    s464_used[idx] = 1;
    return 0;
}

int s464_commit(void) {
    u64 i;
    if (!s464_open) return -1;
    for (i = 0; i < s464_np; i++) {
        if (!s464_used[i]) return -2; /* eksik sayfa */
    }
    s464_open = 0;
    s464_committed = 1;
    return 0;
}

int s464_restore_verify(void) {
    u64 i, j, sum = 0;
    if (!s464_committed) return -1;
    for (i = 0; i < s464_np; i++) {
        if (!s464_used[i]) return -2;
        for (j = 0; j < S464_PAGESZ; j++) sum += s464_pages[i][j];
    }
    return sum == s464_sum ? 0 : -3;
}
