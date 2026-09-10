# 4F - Advanced Memory Entegrasyon Testleri

## Kapsam
- SecureBoot ölçümü sonrası advmem init sırası.
- TPM PCR okuma ile bellek ölçüm korelasyonu (davranışsal).
- PMM+VMM+huge zinciri: alloc → huge-map → kasan-check → reclaim.

## Senaryolar
- 4F2 init sırası, 4F3 zincir, 4F4 hata yayılımı (huge-map hatası reclaim tetiklemez).

## Sonuç
PASS.
