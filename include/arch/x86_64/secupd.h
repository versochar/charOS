#ifndef SECUPD64_H
#define SECUPD64_H
#include "arch/x86_64/longmode.h"

#define SECUPD64_MAX_KEYS 16
#define SECUPD64_NAME_MAX  48

enum secupd64_policy {
    SECUPD_POLICY_RELAXED = 0,  /* imza yoksa da kabul (geliştirme) */
    SECUPD_POLICY_ENFORCED = 1, /* imza zorunlu, dogrulanmadan apply red */
    SECUPD_POLICY_STRICT   = 2, /* imza + anahtar yalnizca SIGPY key id */
};

int secupd64_init(void);
int secupd64_set_policy(int policy);
int secupd64_add_key(int id, u64 secret);
u64 secupd64_sign(const char *name, const char *ver, u64 payload_hash,
                  int key_id);
int secupd64_stage(const char *name, const char *ver, u64 payload_hash,
                   u64 sig, int key_id);
int secupd64_verify(const char *name, const char *ver);
int secupd64_apply_ok(const char *name, const char *ver);
int secupd64_policy(int *out);
int secupd64_staged_count(void);

#endif