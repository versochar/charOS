/* 50G: LUKS */
#include "arch/x86_64/longmode.h"
#include <stddef.h>
static int luks_open = 0;
static const char *luks_cipher = NULL;
int luks64_format(const char *cipher){ luks_cipher = cipher; return 0; }
int luks64_add_key(int slot, u64 hash){ (void)slot; (void)hash; return 0; }
int luks64_unlock(int slot, u64 hash){ (void)slot; (void)hash; luks_open = 1; return 0; }
int luks64_lock(void){ luks_open = 0; return 0; }
int luks64_is_open(void){ return luks_open; }
