# 9F - Drivers & HAL Entegrasyon Testleri

## Kapsam
- proc spawn → drv register → sync mutex korumalı ioctl zinciri.
- Hata yayılımı: geçersiz drv işlemleri diğer tabloları bozmaz.

## Senaryolar
9F2 init sırası, 9F3 register-ioctl zinciri, 9F4 izolasyon.

## Sonuç
PASS.
