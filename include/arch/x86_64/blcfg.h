#ifndef BLCFG64_H
#define BLCFG64_H
#include "arch/x86_64/longmode.h"

#define BLCFG64_MAX_ENTRIES 16
#define BLCFG64_PATH_MAX    64

int blcfg64_init(void);
int blcfg64_set_default(int index);
int blcfg64_set_timeout(int seconds);
int blcfg64_add_entry(const char *label, const char *kernel_path,
                      const char *initrd_path, const char *options);
int blcfg64_default_entry(void);
int blcfg64_validate(const char *kernel_path, const char *initrd_path);
int blcfg64_entry_count(void);
int blcfg64_timeout(int *out);

#endif