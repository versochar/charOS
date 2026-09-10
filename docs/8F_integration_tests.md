# 8F - Synchronization Entegrasyon Testleri

## Kapsam
- proc spawn → mutex lock/unlock zinciri.
- ipc create → spin korumalı send zinciri (davranışsal).
- Hata yayılımı: geçersiz sync işlemleri diğer tabloları bozmaz.

## Senaryolar
8F2 init sırası, 8F3 proc-mutex zinciri, 8F4 spin-ipc zinciri.

## Sonuç
PASS.
