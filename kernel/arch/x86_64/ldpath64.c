/* 53B: LD_LIBRARY_PATH — liste cozumu + $ORIGIN + dosya bulma. */
#include "arch/x86_64/longmode.h"

int ldpath64_parse(const char *list, char out[][96], int max) {
    const char *p;
    int n = 0;
    if (!out || max <= 0) return -1;
    if (!list) return 0;
    p = list;
    while (*p && n < max) {
        int i = 0;
        while (*p == ':') p++;
        if (!*p) break;
        while (*p && *p != ':' && i < 95) out[n][i++] = *p++;
        out[n][i] = 0;
        if (i) n++;
    }
    return n;
}

/* $ORIGIN ve ${ORIGIN} genisletme. */
int ldpath64_expand(const char *path, const char *origin, char *out,
                    int max) {
    int o = 0;
    if (!path || !out || max <= 0) return -1;
    if (!origin) origin = "";
    while (*path && o + 1 < max) {
        if (path[0] == '$' && (path[1] == 'O' || path[1] == '{')) {
            int k = 0;
            const char *q = path + 1;
            /* "ORIGIN" eslesiyor mu? */
            if (q[0] == 'O' && q[1] == 'R' && q[2] == 'I' &&
                q[3] == 'G' && q[4] == 'I' && q[5] == 'N') {
                while (origin[k] && o + 1 < max) out[o++] = origin[k++];
                path += 7;
                continue;
            }
            if (q[0] == '{') {
                int j = 1;
                while (q[j] && q[j] != '}' && j < 8) j++;
                if (q[j] == '}' && j == 7) {
                    /* "${ORIGIN}" uzunlugu 9 */
                    while (origin[k] && o + 1 < max) out[o++] = origin[k++];
                    path += 9;
                    continue;
                }
            }
        }
        out[o++] = *path++;
    }
    out[o] = 0;
    return 0;
}

int ldpath64_find(const char *list, const char *lib, char *out, int max) {
    char dirs[8][96];
    int n, i;
    if (!lib) return -1;
    n = ldpath64_parse(list, dirs, 8);
    if (n < 0) return -1;
    for (i = 0; i < n; i++) {
        /* Yan-etkisiz varlık sorgusu (tabloyu kirletmez) */
        if (rpath64_has(dirs[i], lib)) {
            if (out && max > 0) {
                int k = 0, j;
                for (j = 0; dirs[i][j] && k + 1 < max; j++)
                    out[k++] = dirs[i][j];
                if (k + 1 < max) out[k++] = '/';
                for (j = 0; lib[j] && k + 1 < max; j++)
                    out[k++] = lib[j];
                out[k] = 0;
            }
            return 0;
        }
    }
    return -2;
}
