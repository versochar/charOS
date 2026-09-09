/* 50E: NFS sunucu — disari-aktarim + islem yonlendirme (stub sonuclar). */
#include "arch/x86_64/longmode.h"

#define NFSSERVER64_MAX_EXPORTS 8

#define NFS_PROC_NULL 0
#define NFS_PROC_GETATTR 1
#define NFS_PROC_LOOKUP 3
#define NFS_PROC_READ 6
#define NFS_PROC_WRITE 7

struct nfsserver64_export {
    int used;
    char path[64];
    char clients[64];
};

static struct nfsserver64_export nfsserver64_tab[NFSSERVER64_MAX_EXPORTS];

static void nfs_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int nfs_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int nfsserver64_export(const char *path, const char *clients) {
    int i;
    if (!path || path[0] != '/') return -1;
    for (i = 0; i < NFSSERVER64_MAX_EXPORTS; i++) {
        if (!nfsserver64_tab[i].used) {
            nfsserver64_tab[i].used = 1;
            nfs_str_copy(nfsserver64_tab[i].path, path, 64);
            nfs_str_copy(nfsserver64_tab[i].clients, clients ? clients : "*",
                         64);
            return 0;
        }
    }
    return -2;
}

/* Donus NFS durumu (0=ok). result: LOOKUP->sahte fh, READ/WRITE->bayt. */
int nfsserver64_dispatch(u32 proc, u64 arg, u64 *result) {
    u64 r = 0;
    int st = 0;
    switch (proc) {
    case NFS_PROC_NULL:
        r = 0;
        break;
    case NFS_PROC_GETATTR:
        r = 0100644; /* kip */
        break;
    case NFS_PROC_LOOKUP:
        if (!arg) {
            st = 2; /* ENOENT */
            break;
        }
        r = 0x1000 + (arg & 0xFFF); /* sahte fh */
        break;
    case NFS_PROC_READ:
    case NFS_PROC_WRITE:
        r = 1024; /* skeleton sayfa */
        break;
    default:
        st = 10006; /* PROC_UNAVAIL */
        break;
    }
    if (result) *result = r;
    return st;
}

int nfsserver64_clients(const char *path) {
    int i;
    if (!path) return -1;
    for (i = 0; i < NFSSERVER64_MAX_EXPORTS; i++) {
        if (nfsserver64_tab[i].used &&
            nfs_str_eq(nfsserver64_tab[i].path, path))
            return 1; /* disari aktariliyor */
    }
    return 0;
}
