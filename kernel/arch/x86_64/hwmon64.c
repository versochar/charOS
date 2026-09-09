/* 49A: hwmon core — sensor kaydi + okuma/guncelleme. */
#include "arch/x86_64/longmode.h"

#define HWMON64_MAX 64

struct hwmon64_sensor {
    int used;
    char name[32];
    int type;
    int value;
};

static struct hwmon64_sensor hwmon64_tab[HWMON64_MAX];

static void hwmon_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int hwmon_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

int hwmon64_add(const char *name, int type) {
    int i;
    if (!name || type < 0 || type > 3) return -1;
    for (i = 0; i < HWMON64_MAX; i++) {
        if (hwmon64_tab[i].used && hwmon_str_eq(hwmon64_tab[i].name, name)) {
            hwmon64_tab[i].type = type;
            return 0;
        }
    }
    for (i = 0; i < HWMON64_MAX; i++) {
        if (!hwmon64_tab[i].used) {
            hwmon64_tab[i].used = 1;
            hwmon_str_copy(hwmon64_tab[i].name, name, 32);
            hwmon64_tab[i].type = type;
            hwmon64_tab[i].value = 0;
            return 0;
        }
    }
    return -2;
}

int hwmon64_update(const char *name, int value) {
    int i;
    if (!name) return -1;
    for (i = 0; i < HWMON64_MAX; i++) {
        if (hwmon64_tab[i].used && hwmon_str_eq(hwmon64_tab[i].name, name)) {
            hwmon64_tab[i].value = value;
            return 0;
        }
    }
    return -2;
}

int hwmon64_read(const char *name, int *value) {
    int i;
    if (!name) return -1;
    for (i = 0; i < HWMON64_MAX; i++) {
        if (hwmon64_tab[i].used && hwmon_str_eq(hwmon64_tab[i].name, name)) {
            if (value) *value = hwmon64_tab[i].value;
            return 0;
        }
    }
    return -2;
}
