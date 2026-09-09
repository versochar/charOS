/* 56E: dosya yoneticisi — liste (dizin-oncelikli) + secim + pano. */
#include "arch/x86_64/longmode.h"

#define FILEMAN64_MAX 256
#define FILEMAN64_MAX_CLIP 32

struct fileman64_entry {
    int used;
    char path[96];
    int is_dir;
    u64 size;
    u64 mtime;
};

static int fileman64_sort_mode = 0; /* 0=ad, 1=boyut, 2=zaman */

struct fileman64_clip {
    char path[96];
    int cut;
};

static struct fileman64_entry fileman64_tab[FILEMAN64_MAX];
static struct fileman64_clip fileman64_clip[FILEMAN64_MAX_CLIP];
static int fileman64_nclip = 0;
static char fileman64_selected[96];

static void fileman_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int fileman_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

/* path, dir'in dogrudan cocugu mu? (tek seviye) */
static int fileman_child_of(const char *path, const char *dir,
                            char *base_out) {
    int dl = 0, i = 0;
    while (dir[dl]) dl++;
    /* Kök "/" ozel */
    if (dl == 1 && dir[0] == '/') {
        if (path[0] != '/') return 0;
        i = 1;
    } else {
        int k;
        for (k = 0; k < dl; k++) {
            if (path[k] != dir[k]) return 0;
        }
        if (path[dl] != '/') return 0;
        i = dl + 1;
    }
    {
        int j = 0, slash = 0;
        while (path[i]) {
            if (path[i] == '/') slash = 1;
            if (base_out && j < 63) base_out[j] = path[i];
            i++;
            j++;
        }
        if (slash) return 0; /* torun, dogrudan cocuk degil */
        if (base_out) base_out[j] = 0;
        return j ? 1 : 0;
    }
}

int fileman64_add(const char *path, int is_dir, u64 size) {
    int i;
    if (!path) return -1;
    for (i = 0; i < FILEMAN64_MAX; i++) {
        if (fileman64_tab[i].used &&
            fileman_str_eq(fileman64_tab[i].path, path)) {
            fileman64_tab[i].is_dir = is_dir ? 1 : 0;
            fileman64_tab[i].size = size;
            return 0;
        }
    }
    for (i = 0; i < FILEMAN64_MAX; i++) {
        if (!fileman64_tab[i].used) {
            fileman64_tab[i].used = 1;
            fileman_str_copy(fileman64_tab[i].path, path, 96);
            fileman64_tab[i].is_dir = is_dir ? 1 : 0;
            fileman64_tab[i].size = size;
            return 0;
        }
    }
    return -2;
}

/* Sirali liste: kip 0=ad (dizin-oncelikli), 1=boyut (buyuk once),
 * 2=zaman (yeni once). */
int fileman64_list(const char *dir, char out[][64], int max) {
    struct {
        char base[64];
        int is_dir;
        u64 size;
        u64 mtime;
    } tmp[FILEMAN64_MAX];
    int n = 0, i, j;
    if (!dir || !out || max <= 0) return -1;
    for (i = 0; i < FILEMAN64_MAX && n < FILEMAN64_MAX; i++) {
        if (!fileman64_tab[i].used) continue;
        if (fileman_child_of(fileman64_tab[i].path, dir,
                             tmp[n].base)) {
            tmp[n].is_dir = fileman64_tab[i].is_dir;
            tmp[n].size = fileman64_tab[i].size;
            tmp[n].mtime = fileman64_tab[i].mtime;
            n++;
        }
    }
    for (i = 0; i < n; i++) {
        for (j = i + 1; j < n; j++) {
            int swap = 0, k;
            if (fileman64_sort_mode == 1) {
                /* Boyut: buyuk once (dizinler yine onde) */
                if (tmp[j].is_dir && !tmp[i].is_dir)
                    swap = 1;
                else if (tmp[j].is_dir == tmp[i].is_dir &&
                         tmp[j].size > tmp[i].size)
                    swap = 1;
            } else if (fileman64_sort_mode == 2) {
                /* Zaman: yeni once */
                if (tmp[j].mtime > tmp[i].mtime) swap = 1;
            } else if (tmp[j].is_dir && !tmp[i].is_dir) {
                swap = 1;
            } else if (tmp[j].is_dir == tmp[i].is_dir) {
                for (k = 0;; k++) {
                    if (tmp[j].base[k] != tmp[i].base[k]) {
                        if ((unsigned char)tmp[j].base[k] <
                            (unsigned char)tmp[i].base[k])
                            swap = 1;
                        break;
                    }
                    if (!tmp[j].base[k]) break;
                }
            }
            if (swap) {
                char tb[64];
                int td = tmp[j].is_dir, t2;
                u64 ts, tm;
                for (t2 = 0; t2 < 64; t2++) tb[t2] = tmp[j].base[t2];
                for (t2 = 0; t2 < 64; t2++)
                    tmp[j].base[t2] = tmp[i].base[t2];
                for (t2 = 0; t2 < 64; t2++)
                    tmp[i].base[t2] = tb[t2];
                tmp[j].is_dir = tmp[i].is_dir;
                tmp[i].is_dir = td;
                ts = tmp[j].size;
                tmp[j].size = tmp[i].size;
                tmp[i].size = ts;
                tm = tmp[j].mtime;
                tmp[j].mtime = tmp[i].mtime;
                tmp[i].mtime = tm;
            }
        }
    }
    if (n > max) n = max;
    for (i = 0; i < n; i++) {
        int k;
        for (k = 0; k < 64; k++) {
            out[i][k] = tmp[i].base[k];
            if (!tmp[i].base[k]) break;
        }
    }
    return n;
}

int fileman64_select(const char *path) {
    if (!path) return -1;
    fileman_str_copy(fileman64_selected, path, 96);
    return 0;
}

int fileman64_clip_add(const char *path, int cut) {
    if (!path) return -1;
    if (fileman64_nclip >= FILEMAN64_MAX_CLIP) return -2;
    fileman_str_copy(fileman64_clip[fileman64_nclip].path, path, 96);
    fileman64_clip[fileman64_nclip].cut = cut ? 1 : 0;
    fileman64_nclip++;
    return 0;
}

int fileman64_clip_paste(const char *dir) {
    int i, n = 0;
    if (!dir) return -1;
    for (i = 0; i < fileman64_nclip; i++) {
        /* Skeleton: hedef kaydi olustur (icerik kopyasi 57x'te) */
        char dst[96];
        int a = 0;
        const char *base = fileman64_clip[i].path;
        while (base[a]) a++; /* sona git */
        while (a > 0 && base[a - 1] != '/') a--;
        {
            int p = 0, q;
            while (dir[p] && p < 90) {
                dst[p] = dir[p];
                p++;
            }
            if (p > 0 && dst[p - 1] != '/' && p < 95) dst[p++] = '/';
            for (q = a; base[q] && p < 95; q++, p++) dst[p] = base[q];
            dst[p] = 0;
        }
        if (fileman64_add(dst, 0, 0) == 0) n++;
        if (fileman64_clip[i].cut) {
            /* Tasima: kaynagi kaldir */
            int m;
            for (m = 0; m < FILEMAN64_MAX; m++) {
                if (fileman64_tab[m].used &&
                    fileman_str_eq(fileman64_tab[m].path,
                                   fileman64_clip[i].path)) {
                    fileman64_tab[m].used = 0;
                    break;
                }
            }
        }
    }
    fileman64_nclip = 0;
    return n;
}

int fileman64_mkdir(const char *path) {
    return fileman64_add(path, 1, 0);
}

static struct fileman64_entry *fileman64_find(const char *path) {
    int i;
    for (i = 0; i < FILEMAN64_MAX; i++) {
        if (!fileman64_tab[i].used) continue;
        {
            int k, same = 1;
            for (k = 0;; k++) {
                if (fileman64_tab[i].path[k] != path[k]) {
                    same = 0;
                    break;
                }
                if (!path[k]) break;
            }
            if (same && !path[k]) return &fileman64_tab[i];
        }
    }
    return 0;
}

int fileman64_stat(const char *path, int *is_dir, u64 *size) {
    struct fileman64_entry *e;
    if (!path) return -1;
    e = fileman64_find(path);
    if (!e) return -2;
    if (is_dir) *is_dir = e->is_dir;
    if (size) *size = e->size;
    return 0;
}

int fileman64_touch(const char *path, u64 mtime) {
    struct fileman64_entry *e;
    if (!path) return -1;
    e = fileman64_find(path);
    if (!e) return -2;
    e->mtime = mtime;
    return 0;
}

int fileman64_rename(const char *oldp, const char *newp) {
    struct fileman64_entry *e;
    int i;
    if (!oldp || !newp) return -1;
    if (fileman64_find(newp)) return -2; /* hedef dolu */
    e = fileman64_find(oldp);
    if (!e) return -3;
    /* Alt-agac varsa birlikte tasi (dizin) */
    for (i = 0; i < FILEMAN64_MAX; i++) {
        int k = 0, match = 1;
        if (!fileman64_tab[i].used) continue;
        while (oldp[k]) {
            if (fileman64_tab[i].path[k] != oldp[k]) {
                match = 0;
                break;
            }
            k++;
        }
        if (match && fileman64_tab[i].path[k] == '/') {
            char dst[96];
            int a = 0, b = 0;
            while (newp[a] && a < 90) {
                dst[a] = newp[a];
                a++;
            }
            while (fileman64_tab[i].path[k] && a < 95) {
                dst[a++] = fileman64_tab[i].path[k++];
            }
            dst[a] = 0;
            if (fileman64_find(dst)) return -4;
            fileman_str_copy(fileman64_tab[i].path, dst, 96);
        }
    }
    fileman_str_copy(e->path, newp, 96);
    return 0;
}

static int fileman64_has_child(const char *path) {
    int i;
    for (i = 0; i < FILEMAN64_MAX; i++) {
        int k = 0;
        if (!fileman64_tab[i].used) continue;
        while (path[k]) {
            if (fileman64_tab[i].path[k] != path[k]) break;
            k++;
        }
        if (!path[k] && fileman64_tab[i].path[k] == '/') return 1;
    }
    return 0;
}

int fileman64_remove(const char *path, int recursive) {
    struct fileman64_entry *e;
    int i;
    if (!path) return -1;
    e = fileman64_find(path);
    if (!e) return -2;
    if (e->is_dir && fileman64_has_child(path) && !recursive) return -3;
    if (e->is_dir && recursive) {
        for (i = 0; i < FILEMAN64_MAX; i++) {
            int k = 0, match = 1;
            if (!fileman64_tab[i].used) continue;
            while (path[k]) {
                if (fileman64_tab[i].path[k] != path[k]) {
                    match = 0;
                    break;
                }
                k++;
            }
            if (match && (fileman64_tab[i].path[k] == '/' ||
                          fileman64_tab[i].path[k] == 0))
                fileman64_tab[i].used = 0;
        }
        return 0;
    }
    e->used = 0;
    return 0;
}

int fileman64_copy(const char *src, const char *dst) {
    struct fileman64_entry *s;
    int i;
    if (!src || !dst) return -1;
    if (fileman64_find(dst)) return -2;
    s = fileman64_find(src);
    if (!s || s->is_dir) return -3; /* dizin kopyasi 57x'te */
    for (i = 0; i < FILEMAN64_MAX; i++) {
        if (!fileman64_tab[i].used) {
            fileman64_tab[i].used = 1;
            fileman_str_copy(fileman64_tab[i].path, dst, 96);
            fileman64_tab[i].is_dir = 0;
            fileman64_tab[i].size = s->size;
            fileman64_tab[i].mtime = s->mtime;
            return 0;
        }
    }
    return -4;
}

/* Alt-string arama (tum agacta, ozyinelemeli). Donus eslesme adedi. */
int fileman64_search(const char *dir, const char *substr, char out[][64],
                     int max) {
    int i, n = 0;
    int dl = 0;
    if (!dir || !substr || !out || max <= 0) return -1;
    while (dir[dl]) dl++;
    for (i = 0; i < FILEMAN64_MAX && n < max; i++) {
        int k = 0, found = 0, m;
        if (!fileman64_tab[i].used) continue;
        /* dir one Eki? */
        for (m = 0; m < dl; m++) {
            if (fileman64_tab[i].path[m] != dir[m]) break;
        }
        if (m != dl) continue;
        if (fileman64_tab[i].path[dl] != '/' &&
            fileman64_tab[i].path[dl] != 0)
            continue;
        /* alt-string */
        for (k = 0; fileman64_tab[i].path[k]; k++) {
            int j = 0;
            while (substr[j] && fileman64_tab[i].path[k + j] &&
                   fileman64_tab[i].path[k + j] == substr[j])
                j++;
            if (!substr[j]) {
                found = 1;
                break;
            }
        }
        if (!found) continue;
        for (k = 0; fileman64_tab[i].path[k] && k < 63; k++)
            out[n][k] = fileman64_tab[i].path[k];
        out[n][k] = 0;
        n++;
    }
    return n;
}

int fileman64_sort(int mode) {
    if (mode < 0 || mode > 2) return -1;
    fileman64_sort_mode = mode;
    return 0;
}

u64 fileman64_dirsize(const char *dir) {
    int i;
    u64 total = 0;
    int dl = 0;
    if (!dir) return 0;
    while (dir[dl]) dl++;
    for (i = 0; i < FILEMAN64_MAX; i++) {
        int m;
        if (!fileman64_tab[i].used || fileman64_tab[i].is_dir) continue;
        for (m = 0; m < dl; m++) {
            if (fileman64_tab[i].path[m] != dir[m]) break;
        }
        if (m != dl) continue;
        if (fileman64_tab[i].path[dl] != '/' &&
            fileman64_tab[i].path[dl] != 0)
            continue;
        total += fileman64_tab[i].size;
    }
    return total;
}
