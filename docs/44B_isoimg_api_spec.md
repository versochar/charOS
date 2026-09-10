# 44B - Live ISO Build API Spec

## API
```c
int isoimg64_init(void);
int isoimg64_add_file(const char *name, u64 size_bytes);
int isoimg64_set_boot(const char *kernel_path, int boot_mode);
int isoimg64_layout(u64 *total_bytes);
int isoimg64_sector_for(const char *name, u32 *sector);
int isoimg64_checksum(void);
int isoimg64_verify(const char *name);
int isoimg64_file_count(void);
int isoimg64_boot_mode(int *out);
```

## Donus Kurallari
- add_file: NULL -1; dolu -2; boyut 0/asi -3; ayni isim -4.
- set_boot: gecersiz mod -2; path indexte yok -3.
- layout: bos -1; boot secilmemis -2. Cikti = toplam byte.
