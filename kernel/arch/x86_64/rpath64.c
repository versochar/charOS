/* 52F: RPATH/RUNPATH — arama sirasi + dosya tablosu.
 * Sira: RPATH (DT_RPATH, eski) -> LD_LIBRARY_PATH -> RUNPATH -> varsayilan.
 */
#include "arch/x86_64/longmode.h"

#define RPATH64_MAX_FILES 64
#define RPATH64_MAX_DIRS 8

static char rpath64_dirs[RPATH64_MAX_FILES][96];
static char rpath64_libs[RPATH64_MAX_FILES][48];
static int rpath64_n = 0;

static void rpath_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int rpath_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int rpath64_add_file(const char *dir, const char *lib) {
    if (!dir || !lib || rpath64_n >= RPATH64_MAX_FILES) return -1;
    rpath_str_copy(rpath64_dirs[rpath64_n], dir, 96);
    rpath_str_copy(rpath64_libs[rpath64_n], lib, 48);
    rpath64_n++;
    return 0;
}

/* Salt-okunur varlik sorgusu (53B yan-etkisiz arama icin). */
static int rpath64_has_file(const char *dir, const char *lib) {
    int i;
    for (i = 0; i < rpath64_n; i++) {
        if (rpath_str_eq(rpath64_dirs[i], dir) &&
            rpath_str_eq(rpath64_libs[i], lib))
            return 1;
    }
    return 0;
}

int rpath64_has(const char *dir, const char *lib) {
    if (!dir || !lib) return 0;
    return rpath64_has_file(dir, lib);
}

/* ':' ayracli liste icinde ilk eslesme; donus 0=bulundu. */
static int rpath64_search_list(const char *list, const char *lib, char *out,
                               int max) {
    const char *p;
    if (!list) return -1;
    p = list;
    while (*p) {
        char dir[96];
        int i = 0;
        while (*p == ':') p++;
        if (!*p) break;
        while (*p && *p != ':' && i < 95) dir[i++] = *p++;
        dir[i] = 0;
        if (!dir[0]) continue;
        /* $ORIGIN sadelestirmesi: oldugu gibi karsilastir */
        if (rpath64_has(dir, lib)) {
            if (out && max > 0) rpath_str_copy(out, dir, max);
            return 0;
        }
    }
    return -1;
}

int rpath64_find(const char *rpath, const char *runpath,
                 const char *ldpath, const char *lib, char *out, int max) {
    static const char *defaults[] = {"/lib", "/usr/lib", 0};
    int i;
    if (!lib) return -1;
    if (!rpath64_search_list(rpath, lib, out, max)) return 0;
    if (!rpath64_search_list(ldpath, lib, out, max)) return 0;
    if (!rpath64_search_list(runpath, lib, out, max)) return 0;
    for (i = 0; defaults[i]; i++) {
        if (rpath64_has(defaults[i], lib)) {
            if (out && max > 0) rpath_str_copy(out, defaults[i], max);
            return 0;
        }
    }
    return -2;
}
