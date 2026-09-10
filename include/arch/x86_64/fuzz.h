#ifndef FUZZ64_H
#define FUZZ64_H
#include "arch/x86_64/longmode.h"

#define FUZZ64_MAX_CORPUS  32
#define FUZZ64_BUF_MAX     1024
#define FUZZ64_MAX_CRASHES 16

#define FUZZ64_INPUT_MAX 4096

int fuzz64_init(void);
int fuzz64_corpus_add(const u8 *seed, u16 len);
int fuzz64_corpus_add_file(const char *path);
int fuzz64_iteration(u8 *out, u16 max_out, u16 *out_len);
int fuzz64_feed_crash(const u8 *input, u16 len, u32 reason);
int fuzz64_crash_count(void);
int fuzz64_new_coverage(void); /* artan yol kapsama sayaci */
int fuzz64_xstart(u32 seed);
u32  fuzz64_xnext(void);
int fuzz64_mutate(u8 *buf, u16 *len, u16 max_len);
int fuzz64_token_insert(u8 *buf, u16 *len, u16 max_len, const u8 *token, u16 tlen);

#endif