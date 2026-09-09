/* 52I: dlopen/dlsym — tutamac tablosu + sembol kaydi + sayac. */
#include "arch/x86_64/longmode.h"

#define DLOPEN64_MAX_HANDLE 16
#define DLOPEN64_MAX_SYMS 64

struct dlopen64_sym {
    int used;
    char name[48];
    u64 addr;
};

struct dlopen64_handle {
    int used;
    char path[96];
    u64 bias;
    int global;
    int refcount;
    struct dlopen64_sym syms[DLOPEN64_MAX_SYMS];
};

static struct dlopen64_handle dlopen64_tab[DLOPEN64_MAX_HANDLE];

static void dl_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int dl_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int dlopen64_open(const char *path, int global) {
    int i;
    if (!path) return -1;
    for (i = 0; i < DLOPEN64_MAX_HANDLE; i++) {
        if (dlopen64_tab[i].used &&
            dl_str_eq(dlopen64_tab[i].path, path)) {
            dlopen64_tab[i].refcount++;
            return i;
        }
    }
    for (i = 0; i < DLOPEN64_MAX_HANDLE; i++) {
        int k;
        if (!dlopen64_tab[i].used) {
            dlopen64_tab[i].used = 1;
            dl_str_copy(dlopen64_tab[i].path, path, 96);
            dlopen64_tab[i].bias = 0;
            dlopen64_tab[i].global = global ? 1 : 0;
            dlopen64_tab[i].refcount = 1;
            for (k = 0; k < DLOPEN64_MAX_SYMS; k++)
                dlopen64_tab[i].syms[k].used = 0;
            return i;
        }
    }
    return -2;
}

int dlopen64_add_sym(int h, const char *name, u64 addr) {
    int i;
    if (h < 0 || h >= DLOPEN64_MAX_HANDLE || !dlopen64_tab[h].used)
        return -1;
    if (!name || !addr) return -1;
    for (i = 0; i < DLOPEN64_MAX_SYMS; i++) {
        if (!dlopen64_tab[h].syms[i].used) {
            dlopen64_tab[h].syms[i].used = 1;
            dl_str_copy(dlopen64_tab[h].syms[i].name, name, 48);
            dlopen64_tab[h].syms[i].addr = addr;
            return 0;
        }
    }
    return -2;
}

/* Once tutamacta, sonra global tutamaclarda ara. 0=bulunamadi. */
u64 dlopen64_sym(int h, const char *name) {
    int i, k;
    if (!name) return 0;
    if (h >= 0 && h < DLOPEN64_MAX_HANDLE && dlopen64_tab[h].used) {
        for (k = 0; k < DLOPEN64_MAX_SYMS; k++) {
            if (dlopen64_tab[h].syms[k].used &&
                dl_str_eq(dlopen64_tab[h].syms[k].name, name))
                return dlopen64_tab[h].syms[k].addr;
        }
    }
    for (i = 0; i < DLOPEN64_MAX_HANDLE; i++) {
        if (i == h || !dlopen64_tab[i].used || !dlopen64_tab[i].global)
            continue;
        for (k = 0; k < DLOPEN64_MAX_SYMS; k++) {
            if (dlopen64_tab[i].syms[k].used &&
                dl_str_eq(dlopen64_tab[i].syms[k].name, name))
                return dlopen64_tab[i].syms[k].addr;
        }
    }
    return 0;
}

int dlopen64_close(int h) {
    if (h < 0 || h >= DLOPEN64_MAX_HANDLE || !dlopen64_tab[h].used)
        return -1;
    if (--dlopen64_tab[h].refcount <= 0) {
        dlopen64_tab[h].used = 0;
        dlopen64_tab[h].refcount = 0;
    }
    return 0;
}
