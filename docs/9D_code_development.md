# 9D - Drivers & HAL Kod Geliştirme

## Geliştirmeler
- Sürücü tablosu (64 giriş): handle, tip, state.
- `register`: tip kontrollü slot tahsisi, monoton handle.
- `ioctl`: last-cmd kaydı, bilinmeyen handle reddi.
- Tüm hata yolları negatif testli.

## Not
Gerçek HAL'de MMIO base + IRQ kaydı gerekir (not).

## Test
9D2-9D4 host testleri PASS.

## Sonraki Adım
9E unit test yazma.
