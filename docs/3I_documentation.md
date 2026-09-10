# 3I - Memory Management Dokümantasyon

## Kullanıcı Kılavuzu
PMM, VMM, KHeap kullanımı

## API Referansı
pmm64_init, pmm64_alloc_page, pmm64_free_page
vmm64_map, vmm64_unmap
kheap64_alloc, kheap64_free

## Örnek
```c
pmm64_init();
void *p = pmm64_alloc_page();
pmm64_free_page(p);
```

## Durum
Tamamlandı
