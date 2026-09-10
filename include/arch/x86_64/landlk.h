#ifndef LANDLK64_H
#define LANDLK64_H
#include "arch/x86_64/longmode.h"

#define LANDLK64_MAX_RS     32
#define LANDLK64_MAX_RULES  64
#define LANDLK64_PATH_MAX   96

int landlk64_create_ruleset(u32 handled_access, int *out_id);
int landlk64_add_path_rule(int rs_id, const char *path, u32 allowed_access);
int landlk64_restrict_self(int rs_id);
int landlk64_check(const char *path, u32 access);
int landlk64_handled(int rs_id, u32 *out);
int landlk64_rule_count(int rs_id, int *out);
int landlk64_is_restricted(int rs_id, int *out);

#endif