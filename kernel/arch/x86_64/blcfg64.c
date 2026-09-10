#include "arch/x86_64/blcfg.h"
#include <string.h>

#define BLCFG64_MAX_TIMEOUT 300

struct bl_entry {
    char label[BLCFG64_PATH_MAX];
    char kernel[BLCFG64_PATH_MAX];
    char initrd[BLCFG64_PATH_MAX];
    char options[BLCFG64_PATH_MAX * 2];
    u8   ok;
};
static struct bl_entry entries[BLCFG64_MAX_ENTRIES];
static int entry_n = 0;
static int default_idx = 0;
static int timeout_s = 10;
static int cfg_inited = 0;

/* kernel path: /boot/ ile baslamali, .bin/.elf ile bitmeli (model)
   ve ".." (yol cikisi) icermemeli */
static int path_ok(const char *p) {
    size_t n, i;
    if (!p) return 0;
    if (strncmp(p, "/boot/", 6) != 0) return 0;
    n = strlen(p);
    if (n < 5) return 0;
    for (i = 0; i + 1 < n; i++) {
        if (p[i] == '.' && p[i + 1] == '.') return 0; /* traversal */
    }
    if (strcmp(p + n - 4, ".bin") == 0) return 1;
    if (strcmp(p + n - 4, ".elf") == 0) return 1;
    return 0;
}

int blcfg64_init(void) {
    memset(entries, 0, sizeof(entries));
    entry_n = 0;
    default_idx = 0;
    timeout_s = 10;
    cfg_inited = 1;
    return 0;
}

int blcfg64_set_default(int index) {
    if (!cfg_inited) return -1;
    if (index < 0 || index >= entry_n) return -2;
    default_idx = index;
    return 0;
}

int blcfg64_set_timeout(int seconds) {
    if (!cfg_inited) return -1;
    if (seconds < 0 || seconds > BLCFG64_MAX_TIMEOUT) return -3;
    timeout_s = seconds;
    return 0;
}

int blcfg64_add_entry(const char *label, const char *kernel_path,
                      const char *initrd_path, const char *options) {
    struct bl_entry *e;
    if (!label || !kernel_path) return -1;
    if (!cfg_inited) return -1;
    if (entry_n >= BLCFG64_MAX_ENTRIES) return -2;
    if (!path_ok(kernel_path)) return -3;
    if (initrd_path && strncmp(initrd_path, "/boot/", 6) != 0) return -4;
    e = &entries[entry_n];
    memset(e, 0, sizeof(*e));
    strncpy(e->label, label, sizeof(e->label) - 1);
    strncpy(e->kernel, kernel_path, sizeof(e->kernel) - 1);
    if (initrd_path)
        strncpy(e->initrd, initrd_path, sizeof(e->initrd) - 1);
    if (options)
        strncpy(e->options, options, sizeof(e->options) - 1);
    e->ok = 1;
    entry_n++;
    return 0;
}

int blcfg64_default_entry(void) {
    if (!cfg_inited || entry_n == 0) return -1;
    return default_idx;
}

int blcfg64_validate(const char *kernel_path, const char *initrd_path) {
    if (!cfg_inited) return -1;
    if (!path_ok(kernel_path)) return -2;
    if (initrd_path && strncmp(initrd_path, "/boot/", 6) != 0) return -3;
    return 0;
}

int blcfg64_entry_count(void) {
    if (!cfg_inited) return -1;
    return entry_n;
}

int blcfg64_timeout(int *out) {
    if (!out || !cfg_inited) return -1;
    *out = timeout_s;
    return 0;
}