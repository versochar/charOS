#ifndef SECPOL64_H
#define SECPOL64_H
#include "arch/x86_64/longmode.h"

#define SECPOL64_MAX_PROFILES   32
#define SECPOL64_MAX_RULES      96
#define SECPOL64_NAME_MAX       48
#define SECPOL64_PATH_MAX       96
#define SECPOL64_CTX_MAX        64

#define SECPOL_MODE_ENFORCE 1
#define SECPOL_MODE_APPARMOR 1   /* yol tabanli */
#define SECPOL_MODE_SELINUX  2   /* baglam tabanli */
#define SECPOL_MODE_COMPLAIN 0

#define SECPOL_OP_READ  0x1u
#define SECPOL_OP_WRITE 0x2u
#define SECPOL_OP_EXEC  0x4u

int secpol64_init(void);
int secpol64_add_apparmor(const char *name, const char *path_prefix, int enforce);
int secpol64_add_rule(const char *profile, const char *path, u32 ops, int selinux, u32 *rule_id);
int secpol64_check(const char *profile, const char *path, u32 op);
int secpol64_set_mode(const char *profile, int enforce);
int secpol64_count(void);
int secpol64_profile_count(void);

#endif