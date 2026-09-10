# 47B - LUKS Integration API Spec

## API
```c
int luksop64_init(void);
int luksop64_format(u64 device, const char *cipher, int key_bits);
int luksop64_add_key_slot(int slot, u64 kdf_iter, u64 key_material);
int luksop64_verify_key(int slot, u64 key_material);
int luksop64_unlock(int slot, u64 key_material);
int luksop64_lock(void);
int luksop64_state(int *out);
int luksop64_header_crc(u64 device);
```

## Donus Kurallari
- format: acikken -2; key_bits 128/256/512 degilse -3.
- add_key_slot: slot 0-7 disi -1; kdf<1000 -4; key 0 -5; slot dolu -6.
- verify: bos slot -2; eslesmez -3. unlock: zaten acik -3, hatali anahtar -4.
