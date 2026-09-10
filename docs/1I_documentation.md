# 1I - Secure Boot Dokümantasyon v2.0

## Kullanıcı Kılavuzu
Secure Boot modülü nasıl kullanılır

## API Referansı
secureboot64_init
secureboot64_verify_signature
secureboot64_measure_kernel
secureboot64_log_event
secureboot64_get_last_event
secureboot64_get_pcr

## Örnek Kullanım
```c
secureboot64_init();
u8 data[4] = {...};
u8 sig[512] = {...};
if (secureboot64_verify_signature(data,4,sig,512) == SB_OK) {
    secureboot64_measure_kernel(data,4);
}
```

## Kurulum
Derleme: make test-de64

## Destek
charOS Core Team
