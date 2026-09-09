# 1B - Core Architecture & Secure Boot - API Spesifikasyonu

## Kapsam
1A tasarımının API sözleşmesi. UEFI bootloader ve kernel arasındaki güvenli başlatma arayüzü.

## API Modülü: secureboot

### Fonksiyonlar

#### int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len)
- **Amaç**: Kernel/PE imzasını doğrula
- **Giriş**: data, len, sig, sig_len
- **Çıkış**: 0 başarı, -1 hata, -2 geçersiz imza
- **Kısıtlar**: Freestanding, no libc, sabit zamanlı karşılaştırma
- **Bağımlılık**: crypto64_verify_pkcs1v15

#### int secureboot64_measure_kernel(const void *kernel, u64 size)
- **Amaç**: Kernel imajını ölç ve PCR'ye ekle
- **Giriş**: kernel adresi, size
- **Çıkış**: ölçüm kimliği veya -1
- **Not**: SHA-256 kullan

#### void secureboot64_log_event(const char *event)
- **Amaç**: Güvenli olay günlüğüne yaz
- **Kısıt**: ISR güvende, lock-free ring buffer

### Veri Yapıları
```c
typedef struct {
    u8 hash[32];
    u64 pcrsel;
    u64 timestamp;
} secureboot_event_t;
```

### Hata Kodları
0 = OK, -1 = GENEL HATA, -2 = İMZA GEÇERSİZ, -3 = PCR DOLU

### Test Kriterleri
- API prototipleri longmode.h'de mevcut
- Stub implementasyon derleniyor
- Host test 1B1-1B4 PASS

### Sonraki Adım
1C: Implementasyon başlatma
