# 1B - Secure Boot API Spesifikasyonu v2.0

## 1. Giriş
Bu belge charOS Secure Boot modülünün API sözleşmesini tanımlar. Tüm fonksiyonlar freestanding, freestanding-uyumlu, thread-safe değildir, re-entrant değildir.

## 2. Modül Tanımı
Modül adı: secureboot
Sürüm: 1.0
Bağımlılıklar: crypto64, tpm64, log64

## 3. API Sözleşmesi

### 3.1 secureboot64_verify_signature
```c
int secureboot64_verify_signature(const u8 *data, u64 len, const u8 *sig, u64 sig_len);
```
**Açıklama**: RSA-PSS SHA-256 imza doğrulaması yapar.
**Girdi**:
- data: doğrulanacak imaj
- len: imaj boyutu
- sig: imza verisi
- sig_len: imza uzunluğu, RSA-4096 için 512
**Çıkış**:
- 0: başarılı
- -1: parametre hatası
- -2: imza geçersiz
- -3: kripto modülü başlatılmamış
**Önkoşul**: secureboot_init() çağrılmış olmalı
**Sonraki**: Doğrulanmış imaj ölçülmelidir

### 3.2 secureboot64_measure_kernel
```c
int secureboot64_measure_kernel(const void *kernel, u64 size);
```
**Açıklama**: Kernel imajını SHA-256 ile ölçer ve PCR[1]'e genişletir.
**Çıkış**: 0 başarılı, -1 hata
**Yan etki**: TPM PCR değeri güncellenir

### 3.3 secureboot64_log_event
```c
void secureboot64_log_event(const char *event);
```
**Açıklama**: Olayı ring buffer'a yazar

## 4. Veri Yapıları
```c
typedef struct {
    u8 hash[32];
    u64 pcr_index;
    u64 timestamp;
    u8 event_type;
} secureboot_event_t;

typedef enum {
    SB_OK = 0,
    SB_ERR_PARAM = -1,
    SB_ERR_INVALID_SIG = -2,
    SB_ERR_CRYPTO = -3,
} secureboot_err_t;
```

## 5. Hata Yönetimi
Tüm hata kodları belgelenmiştir. Uygulama hata durumunda kernel panic yapmaz, güvenli fallback uygular.

## 6. Thread Safety
API single-threaded boot sırasında kullanılır. Runtime'da kilit gerekmez.

## 7. Test Matrisi
- T1: Geçerli imza -> PASS
- T2: Geçersiz imza -> -2
- T3: NULL parametre -> -1
- T4: Ölçüm tutarlılığı

## 8. Uyumluluk
UEFI Secure Boot DB/KEK/DBX ile uyumlu

## 9. Değişiklik Geçmişi
v1.0 - İlk yayın

## 10. Referanslar
- UEFI Spec 2.8 Sec 27
- TPM 2.0 Part 1
- NIST SP 800-155
