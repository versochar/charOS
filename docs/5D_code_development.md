# 5D - Process Kod Geliştirme

## Geliştirmeler
- Slot tablosu (64 giriş): PID, state, exit code.
- `spawn`: FREE slot bulur, PID=++ctr, state=READY.
- `exit`: READY/RUNNING -> ZOMBIE + kod kaydı; çift exit reddedilir.
- `wait`: ZOMBIE -> kod döndürür + slot FREE.
- Tüm hata yolları negatif testli.

## Not
Gerçek scheduler'da READY->RUNNING geçişi context-switch ile olur; modelde READY varsayılır.

## Test
5D2-5D3 host testleri PASS.

## Sonraki Adım
5E unit test yazma.
