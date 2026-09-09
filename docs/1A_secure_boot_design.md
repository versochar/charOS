# 1A - Core Architecture & Secure Boot - Tasarım ve Mimari Karar

## Amaç
charOS profesyonel kernel için güvenli başlatma zinciri mimarisinin tasarımı.

## Kararlar
- **Boot Modu**: UEFI Secure Boot uyumlu PE imaj + Legacy BIOS fallback
- **Güven Modeli**: Root of Trust -> Bootloader -> Kernel -> Init
- **İmzalama**: Kernel.bin ve BOOTX64.EFI, x509 sertifika ile imzalanacak
- **Önyükleme Akışı**:
  1. UEFI firmware -> BOOTX64.EFI
  2. İmza doğrulama
  3. Kernel.bin yükleme
  4. GDT/IDT kurulumu
  5. Secure measurement log

## Dosya Yapısı
- `boot/uefi/loader.c` - Secure boot loader
- `kernel/core/secureboot64.c` - Ölçüm ve doğrulama
- `include/arch/x86_64/secureboot.h` - API prototipleri

## Başarı Kriterleri
- Tasarım belgesi tamam
- API taslağı oluşturuldu
- Boot akış diyagramı hazır

## Sonraki Adım
1B: API spesifikasyonu
