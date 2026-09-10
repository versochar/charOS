# 6D - Scheduling Kod Geliştirme

## Geliştirmeler
- Görev tablosu (64 giriş): pid, prio, active.
- Seçim: active görevler arası min prio; eşitlikte eklenme sırası (seq).
- `yield/tick`: seçili sınıf içinde FIFO rotasyon (seq yeniden numaralama).
- Tüm hata yolları negatif testli.

## Not
Gerçek preemption'da tick IRQ kilidi + `invlpg`/TLB gerekmez ama runqueue spinlock gerekir (not).

## Test
6D2-6D4 host testleri PASS.

## Sonraki Adım
6E unit test yazma.
