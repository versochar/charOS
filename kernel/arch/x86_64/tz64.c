/* 51F: time zone — yaz-saati kurallari (ay/hafta bazli, ABD+AB). */
#include "arch/x86_64/longmode.h"

#define TZ64_MAX 8

struct tz64_rule {
    int used;
    char name[32];
    int std_min; /* UTC'den dakika (dogu +) */
    int dst_min;
    int sm, sw; /* baslangic: ay, hafta (1-5, 5=son) */
    int em, ew; /* bitis: ay, hafta */
};

static struct tz64_rule tz64_tab[TZ64_MAX];

static void tz_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int tz_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int tz64_add_rule(const char *name, int std_min, int dst_min, int sm,
                  int sw, int em, int ew) {
    int i;
    if (!name || sm < 1 || sm > 12 || em < 1 || em > 12) return -1;
    for (i = 0; i < TZ64_MAX; i++) {
        if (!tz64_tab[i].used) {
            tz64_tab[i].used = 1;
            tz_str_copy(tz64_tab[i].name, name, 32);
            tz64_tab[i].std_min = std_min;
            tz64_tab[i].dst_min = dst_min;
            tz64_tab[i].sm = sm;
            tz64_tab[i].sw = sw;
            tz64_tab[i].em = em;
            tz64_tab[i].ew = ew;
            return 0;
        }
    }
    return -2;
}

static struct tz64_rule *tz64_find(const char *name) {
    int i;
    for (i = 0; i < TZ64_MAX; i++)
        if (tz64_tab[i].used && tz_str_eq(tz64_tab[i].name, name))
            return &tz64_tab[i];
    return 0;
}

/* Yazin-icinde mi? (gun duzeyi yaklasim: ay karsilastirma + hafta yoksay
 * skeleton'da ay+gun esigi; tam tarihe 52x'te.) */
static int tz64_in_dst(const struct tz64_rule *r, int mon, int day) {
    (void)day;
    if (r->sm < r->em)
        return (mon > r->sm && mon < r->em) ||
               (mon == r->sm) || (mon == r->em);
    if (r->sm > r->em)
        return (mon > r->sm || mon < r->em);
    return 0;
}

int tz64_offset(const char *name, int y, int mon, int day, int hour) {
    struct tz64_rule *r = tz64_find(name);
    (void)y;
    (void)hour;
    if (!r || mon < 1 || mon > 12 || day < 1 || day > 31) return 0;
    return tz64_in_dst(r, mon, day) ? r->dst_min : r->std_min;
}

long long tz64_utc_to_local(const char *name, long long utc, int y, int mon,
                            int day, int hour) {
    return utc + (long long)tz64_offset(name, y, mon, day, hour) * 60;
}
