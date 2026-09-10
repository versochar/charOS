# 55A - Kernel Self Protection Design

## Amaç
Kendi kendini koruyan cekirdek (`kprot64`): lockdown seviyeleri, kptr/dmesg kisitlamasi, rodata kilidi, oops panik esigi.

## Kararlar
- **Lockdown** monotonik artar (NONE->CONFIDENTIAL->INTEGRITY->PGD); geri alma -2.
- **kptr modu**: ALL (acik), RESTRICT (yetkisiz maskele), ZERO (sifirlama).
- **rodata kilidi**: yalniz INTEGRITY+ kilidi gerceklesir; kilitliyken yazma -2.
- **oops esigi**: limitin uzerine cikinca panic_required=1.

## Basari Kriterleri
- `make test-de64` 55A-J PASS.
