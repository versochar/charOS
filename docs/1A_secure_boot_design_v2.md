# 1A - Core Architecture & Secure Boot - Tasarım ve Mimari Karar
## Versiyon 2.0 - Profesyonel Kalite

### 1. Executive Summary
charOS için güvenli başlatma zinciri, UEFI Secure Boot uyumlu, ölçümlü ve imzalı bir boot akışı tasarlar. Root of Trust firmware'den başlar, UEFI loader, kernel ve init arasında ölçüm zinciri sağlar.

### 2. Mimari Kararlar
#### 2.1 Boot Modu
- **Primary**: UEFI 2.8 + Secure Boot
- **Fallback**: Legacy BIOS + şifrelenmiş MBR
- **Hedef platform**: x86_64, AMD PSP / Intel TXT uyumlu

#### 2.2 Güven Modeli
```
Firmware RoT -> BOOTX64.EFI -> kernel.bin -> init
      |              |             |         |
   Ölçüm PCR[0]  PCR[1]      PCR[2]    PCR[3]
```
Her aşama bir öncekinin ölçümünü doğrular.

#### 2.3 İmzalama
- Algoritma: RSA-4096 PSS + SHA-256
- Sertifika hiyerarşisi: Root CA -> Platform CA -> Kernel CA
- İmza dosyası: `kernel.bin.sig`

### 3. Bileşenler
#### 3.1 UEFI Loader
- Dosya: `boot/uefi/loader.c`
- Görevler: Sertifika doğrulama, kernel yükleme, PCR ölçümü
- API: `uefi_verify_image()`, `uefi_measure()`

#### 3.2 Kernel Secure Boot Modülü
- Dosya: `kernel/arch/x86_64/secureboot64.c`
- Görevler: Runtime ölçüm, olay günlüğü, TPM raporu
- API: `secureboot64_verify_signature()`, `secureboot64_measure_kernel()`, `secureboot64_log_event()`

#### 3.3 Public Key Infrastructure
- `keys/root.crt`, `keys/platform.key`
- `scripts/sign_kernel.sh`

### 4. Veri Akışı
1. Firmware UEFI loader'ı yükler
2. Loader kernel imzasını doğrular
3. Doğrulama başarılıysa kernel'i belleğe kopyalar
4. PCR[1]'e kernel hash ölçümü eklenir
5. Kernel başlar, `secureboot64_measure_kernel()` ile kendi ölçümünü teyit eder

### 5. Tehdit Modeli
- **Saldırı**: Değiştirilmiş kernel
- **Savunma**: İmza doğrulama + PCR ölçümü
- **Saldırı**: Yetkisiz loader
- **Savunma**: Secure Boot DB whitelist

### 6. Güvenlik Garantileri
- Imza doğrulama başarısız olursa boot durdurulur
- Ölçüm zinciri koparsa kernel panic
- Olay günlüğü salt-okunur

### 7. Performans Hedefleri
- İmza doğrulama < 10ms
- Ölçüm < 1ms

### 8. Test Planı
- Pozitif test: İmzalı kernel boot olur
- Negatif test: İmzası bozuk kernel reddedilir
- TPM PCR değerleri doğrulanır

### 9. Bağımlılıklar
- `crypto64_rsa_verify`
- `tpm64_extend_pcr`
- `log64_ringbuf`

### 10. Sonraki Adımlar
1B: API spesifikasyonu detaylandırıldı
1C: Implementasyon başlatma
1D: Kod geliştirme

### 11. Referanslar
- UEFI Spec 2.8
- TPM 2.0 Library Spec
- NIST SP 800-193

---
Tarih: 2026-09-10
Sorumlu: charOS Core Team
