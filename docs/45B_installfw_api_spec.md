# 45B - Installer Framework API Spec

## API
```c
int installfw64_init(void);
int installfw64_preflight(u64 disk_bytes, u64 ram_bytes);
int installfw64_select_disk(const char *dev, int wipe);
int installfw64_create_partition(const char *label, u64 bytes);
int installfw64_copy_stage(const char *from, const char *to, u64 bytes);
int installfw64_install_bootloader(const char *target);
int installfw64_finalize(void);
int installfw64_step(int *out);
int installfw64_partition_count(void);
```

## Donus Kurallari
- preflight: disk<4GiB -2, ram<512MiB -3, yanlis adim -1.
- select_disk: wipe 0/1 degilse -3.
- partition: <32MiB -3, yanlis adim -2.
- copy: toplam asimi -4.
