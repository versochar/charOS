# 57B - Fuzzing Infrastructure API Spec

## API
```c
int fuzz64_init(void);
int fuzz64_corpus_add(const u8 *seed, u16 len);
int fuzz64_corpus_add_file(const char *path);
int fuzz64_iteration(u8 *out, u16 max_out, u16 *out_len);
int fuzz64_feed_crash(const u8 *input, u16 len, u32 reason);
int fuzz64_crash_count(void);
int fuzz64_new_coverage(void);
int fuzz64_xstart(u32 seed);
u32  fuzz64_xnext(void);
int fuzz64_mutate(u8 *buf, u16 *len, u16 max_len);
int fuzz64_token_insert(u8 *buf, u16 *len, u16 max_len, const u8 *token, u16 tlen);
```

## Donus Kurallari
- corpus_add: null/0 uzunluk -1; dolu -2; file stub -2.
- iteration: null/0 cap -1; bos korpus -2.
- mutate: null/0 cap -1; bos -2.
- token_insert: null -1; 0 token -2; tasma -3.
