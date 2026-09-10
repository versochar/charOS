# 46B - Partitioning Tools API Spec

## API
```c
int parttool64_init(void);
int parttool64_gpt_init(u64 dev_id, u64 total_lba);
int parttool64_gpt_add(u64 dev_id, const char *name, int type_code, u64 first, u64 last);
int parttool64_gpt_info(u64 dev_id, int index, char *name_out, int name_max, int *type_out, u64 *first_out, u64 *last_out);
int parttool64_gpt_delete(u64 dev_id, int index);
int parttool64_gpt_crc(u64 dev_id);
int parttool64_gpt_repair(u64 dev_id);
int parttool64_gpt_count(u64 dev_id);
```

## Donus Kurallari
- gpt_init: total<34 -1; zaten formatli -2; tablo dolu -3.
- gpt_add: hatali aralik -3; koruyucu bolge -4; dolu -5; bozuk tablo -6; cakis -7.
