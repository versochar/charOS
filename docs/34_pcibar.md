# 34 — BAR Altyapısı (gerçek çekirdek kodu)

Şema: harf yok, tek sayıda 10 aşama (34.1–34.10).
Tespit: `pci_read_bar` 64-bit BAR'da üst dword'u okumuyordu (sonraki BAR
kayar) ve boyut yoklama yoktu.

## 34.1 Tasarım
- Kodçözüm (IO/mem32/mem64/prefetch) + yoklama-boyutu saf fonksiyonlarda.
- Boyut formülleri PCI spec: `~(maske)+1`, uygulanmamış BAR=0.

## 34.2 API (`include/drivers/pcibar.h`)
- `pcibar_decode/size32/size64/sizeio/selftest`; yalnızca `stdint.h`.

## 34.3 Mantık (`kernel/drivers/pcibar.c`)
- Maske tekilleşir + 1 derleme-zamanı kanıtı; sözleşmeler `REQUIRE` ile.
- Aynı dosya çekirdekte ve host'ta derlenir.

## 34.4 Entegrasyon
- `pci.c`: `pci_read_bar64()` (64-bit'te üst dword dahil); eski fn korunur.
- `kernel.c` `[34]` bloğu: `pcibar` öztest kaydı + e1000 BAR0 gerçek okuma.
- 34.4-fix: öztestteki 32-bit sabit taşması (`0x200000010u`) host testle
  yakalandı; `ULL` düzeltmesi. Boyut beklentisi de 2^48 ile doğrulandı.

## 34.5 Host test (`tests/host/test_pcibar34.c`, `make test-pcibar34`)
- GERÇEK `kernel/drivers/pcibar.c` derlenir: TUMU PASS (10 kontrol).

## 34.6 Derleme
- `KERNEL_SRCS` += `kernel/drivers/pcibar.c`; `make build/kernel.bin`
  temiz, 6 sembol ELF'te.

## 34.7 Kod inceleme
- [x] 64-bit birleştirme sırası doğru (hi<<32 | lo-masked)
- [x] Yoklama formülleri spec ile uyumlu (maske dışı bitler temizlenir)
- [x] Uygulanmamış BAR (probe 0) boyut 0 döner
- [x] Sabit genişlikleri `ULL` (32-bit kesilme yok)

## 34.8 Güvenlik denetimi notu
- BAR yazma (sizing) bu seride yok — yalnızca okuma + saf hesap
  (donanım durumu değiştirilmez).
- Kötü BAR değeri çekirdeği çökertmez (decode saf, NULL-denetimli).

## 34.9 Dokümantasyon
- Bu dosya + header içi formül açıklamaları.

## 34.10 Sürüm
- Versiyon: 34.0.0. `make test-pcibar34` + çekirdek derlemesi yeşil.
- Dosyalar: `include/drivers/pcibar.h`, `kernel/drivers/pcibar.c`,
  `tests/host/test_pcibar34.c`, `docs/34_pcibar.md`.
