# 6F - Scheduling Entegrasyon Testleri

## Kapsam
- SecureBoot/advmem/proc init sonrası sched init sırası.
- proc spawn → sched add zinciri.
- Hata yayılımı: geçersiz sched işlemleri proc tablosunu bozmaz.

## Senaryolar
6F2 init sırası, 6F3 spawn-add zinciri, 6F4 izolasyon.

## Sonuç
PASS.
