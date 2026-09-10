# 4I - Advanced Memory Dokümantasyon

## Kullanıcı Kılavuzu
```c
advmem64_init();
u64 phys;
advmem64_alloc_node(0, 0, &phys);
advmem64_map_huge(0x200000, phys, 0x3, 1);
advmem64_kasan_poison((void*)0x200000, 4096);
advmem64_kasan_check((void*)0x200000, 4096);
```

## API Referansı
`advmem64_init`, `advmem64_alloc_node`, `advmem64_map_huge`,
`advmem64_kasan_poison`, `advmem64_kasan_check`, `advmem64_reclaim`.

## Not
Host-test davranış modelidir; gerçek donanım mappingi `paging.c` üzerinden yapılacaktır.

## Durum
Tamamlandı.
