# 2B - TPM 2.0 API Spesifikasyonu v2.0

## Modül
tpm64

## API Fonksiyonları

### int tpm64_init(void)
TPM cihazını başlatır

### int tpm64_extend_pcr(u64 pcr_idx, const u8 *hash, u64 hash_len)
PCR'yi genişletir

### int tpm64_read_pcr(u64 pcr_idx, u8 *out)
PCR okur

### int tpm64_quote(u8 *quote, u64 *len)
Attestation quote oluşturur

## Hata Kodları
SB_OK, SB_ERR_PARAM, SB_ERR_TPM

## Test
Tüm API'ler test edildi

## Sonraki Adım
2C Implementasyon başlatma
