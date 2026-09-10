# 5C - Process Implementasyon Başlatma

## Yapılanlar
- `include/arch/x86_64/proc.h` iskeleti tanımlandı (5B API prototipleri).
- Host-test davranış modeli `kernel/arch/x86_64/proc64.c` ile doğrulandı: sabit slot tablosu, monoton PID.
- Gerçek scheduler entegrasyonu için dosya yerleşimi rezerve edildi; mevcut çekirdek derlemesi etkilenmez.

## Dosya Yapısı
```
include/arch/x86_64/proc.h
kernel/arch/x86_64/proc64.c   # host-test davranış modeli
kernel/process/proc_sched.c   # plan (gerçek scheduler)
```

## Derleme
`make test-de64` yeşil; `make all` etkilenmedi.

## Sonraki Adım
5D kod geliştirme.
