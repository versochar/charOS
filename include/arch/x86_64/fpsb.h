#ifndef FPSB64_H
#define FPSB64_H
#include "arch/x86_64/longmode.h"

#define FPSB64_MAX_SANDBOXES 32
#define FPSB64_NAME_MAX      64
#define FPSB64_APP_ID_MAX    48

/* izin bitleri */
#define FPSB_NET           0x0001u
#define FPSB_DBUS          0x0002u
#define FPSB_FILESYS_HOST  0x0004u
#define FPSB_GPU           0x0008u
#define FPSB_X11           0x0010u
#define FPSB_PULSE         0x0020u
#define FPSB_DEVICES       0x0040u

enum fpsb64_state {
    FPSB_CREATED = 0,
    FPSB_RUNNING = 1,
    FPSB_BLOCKED  = 2,
};

int fpsb64_init(void);
int fpsb64_create(const char *app_id, u32 perms, const char *runtime_ref,
                  int *out_id);
int fpsb64_launch(int id);
int fpsb64_grant(int id, u32 bits);
int fpsb64_revoke(int id, u32 bits);
int fpsb64_has_perm(int id, u32 bits);
int fpsb64_state(int id, int *out);
int fpsb64_denied_count(int id, int *out);
int fpsb64_count(void);

#endif