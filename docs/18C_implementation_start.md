# 18C - TCP Implementasyon Başlatma

## Yapılanlar
- `include/arch/x86_64/tcp.h` iskeleti tanımlandı.
- Host-test davranış modeli `kernel/arch/x86_64/tcp64.c` ile doğrulandı.
- Gerçek stack için yerleşim rezerve edildi.

## Dosya Yapısı
```
include/arch/x86_64/tcp.h
kernel/arch/x86_64/tcp64.c
kernel/net/tcp.c   # plan
```

## Derleme
`make test-de64` yeşil.

## Sonraki Adım
18D kod geliştirme.
