# 17C - Network Implementasyon Başlatma

## Yapılanlar
- `include/arch/x86_64/net.h` iskeleti tanımlandı.
- Host-test davranış modeli `kernel/arch/x86_64/net64.c` ile doğrulandı.
- Gerçek stack için yerleşim rezerve edildi.

## Dosya Yapısı
```
include/arch/x86_64/net.h
kernel/arch/x86_64/net64.c
kernel/net/stack.c   # plan
```

## Derleme
`make test-de64` yeşil.

## Sonraki Adım
17D kod geliştirme.
