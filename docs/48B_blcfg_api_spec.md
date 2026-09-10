# 48B - Bootloader Integration API Spec

## API
```c
int blcfg64_init(void);
int blcfg64_set_default(int index);
int blcfg64_set_timeout(int seconds);
int blcfg64_add_entry(const char *label, const char *kernel_path, const char *initrd_path, const char *options);
int blcfg64_default_entry(void);
int blcfg64_validate(const char *kernel_path, const char *initrd_path);
int blcfg64_entry_count(void);
int blcfg64_timeout(int *out);
```

## Donus Kurallari
- add_entry: yok/yonetimsiz -1; dolu -2; kernel path gecersiz -3; initrd gecersiz -4.
- set_timeout: 0..300 disi -3. set_default: gecersiz indeks -2.
