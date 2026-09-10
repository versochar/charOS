# 2A - Core Architecture & Secure Boot - TPM 2.0 Entegrasyon Tasarımı

## Amaç
Secure Boot modülüne TPM 2.0 desteği eklemek, ölçümleri TPM PCR'larına kaydetmek.

## Tasarım
- TPM 2.0 üzerinden PCR[0-7] kullanımı
- Ekstansiyon: PCR = Hash(PCR || ölçüm)
- Attestation desteği

## Bileşenler
- kernel/arch/x86_64/tpm64.c
- include/arch/x86_64/tpm.h

## Başarı Kriterleri
Tasarım dokümanı tamam
API taslağı hazır

## Sonraki Adım
2B API spesifikasyonu
