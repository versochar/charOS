# 8D - Synchronization Kod Geliştirme

## Geliştirmeler
- Spin tablosu (32 giriş): id, locked.
- Mutex tablosu (32 giriş): id, locked, owner.
- `trylock/lock`: double-acquire reddedilir; `unlock`: kilitsiz/yanlış-owner reddedilir.
- Tüm hata yolları negatif testli.

## Not
Gerçek SMP'de `lock cmpxchg` + memory barrier gerekir (not).

## Test
8D2-8D4 host testleri PASS.

## Sonraki Adım
8E unit test yazma.
