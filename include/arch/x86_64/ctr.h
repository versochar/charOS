#ifndef CTR64_H
#define CTR64_H
#include "arch/x86_64/longmode.h"

#define CTR64_MAX_CONTAINERS 32
#define CTR64_NAME_MAX       48
#define CTR64_IMAGE_MAX      64

enum ctr64_state {
    CTR64_CREATED = 0,
    CTR64_RUNNING = 1,
    CTR64_PAUSED  = 2,
    CTR64_STOPPED = 3,
};

int ctr64_init(void);
int ctr64_create(const char *name, const char *image, u64 mem_limit_bytes,
                 int *out_id);
int ctr64_start(int id);
int ctr64_pause(int id);
int ctr64_resume(int id);
int ctr64_stop(int id);
int ctr64_state(int id, int *out);
int ctr64_count(void);
int ctr64_pid(int id, u64 *out_pid);

#endif