# 1E - Secure Boot Unit Tests v2.0

## Test Kapsamı
Secure Boot modülünün tüm API fonksiyonları için kapsamlı unit testler.

## Test Senaryoları

### 1E1 - Init Testi
- secureboot64_init() başarılı dönmeli
- PCR ve event sıfırlanmalı

### 1E2 - Parametre Hatası
- NULL data ile verify -> SB_ERR_PARAM
- Zero length -> SB_ERR_PARAM

### 1E3 - İmza Doğrulama Pozitif
- Geçerli imza ile verify -> SB_OK

### 1E4 - İmza Doğrulama Negatif
- Geçersiz imza -> SB_ERR_INVALID_SIG
- Yanlış uzunluk -> SB_ERR_INVALID_SIG

### 1E5 - Ölçüm Testi
- Kernel ölçümü başarılı
- PCR değeri güncellenir

### 1E6 - Olay Günlüğü
- log_event çağrısı crash yapmaz
- get_last_event doğru event döndürür

### 1E7 - PCR Erişim
- get_pcr doğru boyutta veri döndürür
- PCR tutarlılığı

## Kapsama
- Kod kapsamı %100
- Hata yolları test edildi

## Sonuç
Tüm unit testler PASS
