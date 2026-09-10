# 7D - IPC Kod Geliştirme

## Geliştirmeler
- Kanal tablosu (64 giriş): chan-id, valid flag, msg.
- `send`: boş mailboxa yazar, valid=1; doluysa -1.
- `recv`: valid ise okur + valid=0; boşsa -1.
- Tüm hata yolları negatif testli.

## Not
Gerçek IPC'de çoklu mesaj kuyruğu + waiter listesi gerekir (not).

## Test
7D2-7D4 host testleri PASS.

## Sonraki Adım
7E unit test yazma.
