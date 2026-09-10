# 4B - Advanced Memory API Spesifikasyonu

## API
```c
int pmm64_alloc_node(int node, int order, u64 *out_phys);
int vmm64_map_huge(u64 virt, u64 phys, u64 flags, int level); /* level: 1=2MB, 2=1GB */
void kasan64_poison(const void *addr, u64 size);
int kasan64_check(const void *addr, u64 size); /* 0=temiz, -1=poisoned */
int reclaim64_run(u64 target_pages, u64 *freed);
```

## Sözleşme
- `pmm64_alloc_node`: node < 0 ise any-node. Başarıda 0, bellek yoksa -1.
- `vmm64_map_huge`: hizalanmamış adreslerde -1, çakışmada -2.
- `kasan64_check`: NULL/size 0 ise -1 (parametre hatası).
- `reclaim64_run`: freed NULL ise -1, aksi halde 0 ve freed sayfa sayısı.

## Hata Kodları
0 OK, -1 parametre/kaynak hatası, -2 çakışma.

## Uyumluluk
Mevcut `pmm.c` / `paging.c` API'leri değişmez; yeni API'ler opsiyonel uzantıdır.

## Sonraki Adım
4C implementasyon başlatma.
