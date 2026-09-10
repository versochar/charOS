# 6C - Scheduling Implementasyon Başlatma

## Yapılanlar
- `include/arch/x86_64/sched.h` iskeleti tanımlandı (6B API prototipleri).
- Host-test davranış modeli `kernel/arch/x86_64/sched64.c` ile doğrulandı: sabit görev tablosu, prio sıralı seçim, FIFO rotasyon.
- Gerçek timer IRQ + context-switch entegrasyonu için yerleşim rezerve edildi; mevcut çekirdek derlemesi etkilenmez.

## Dosya Yapısı
```
include/arch/x86_64/sched.h
kernel/arch/x86_64/sched64.c   # host-test davranış modeli
kernel/process/sched_rt.c      # plan (gerçek scheduler)
```

## Derleme
`make test-de64` yeşil; `make all` etkilenmedi.

## Sonraki Adım
6D kod geliştirme.
