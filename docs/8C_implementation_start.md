# 8C - Synchronization Implementasyon Başlatma

## Yapılanlar
- `include/arch/x86_64/sync.h` iskeleti tanımlandı (8B API prototipleri).
- Host-test davranış modeli `kernel/arch/x86_64/sync64.c` ile doğrulandı: slot tablosu, locked flag, owner takibi.
- Gerçek SMP atomik + IRQ-save entegrasyonu için yerleşim rezerve edildi; mevcut çekirdek derlemesi etkilenmez.

## Dosya Yapısı
```
include/arch/x86_64/sync.h
kernel/arch/x86_64/sync64.c   # host-test davranış modeli
kernel/core/spinlock.c        # plan (gerçek SMP)
```

## Derleme
`make test-de64` yeşil; `make all` etkilenmedi.

## Sonraki Adım
8D kod geliştirme.
