# 7F - IPC Entegrasyon Testleri

## Kapsam
- proc spawn → ipc create → sched add zinciri.
- Hata yayılımı: geçersiz IPC işlemleri proc/sched tablolarını bozmaz.

## Senaryolar
7F2 init sırası, 7F3 create-send-recv zinciri, 7F4 izolasyon.

## Sonuç
PASS.
