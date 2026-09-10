# 39A - Crash Dump & Recovery Design

## Amaç
Kernel crash dump & recovery modulu: hata aninda register/stack durumunu kaydet, yeniden baslat.

## Kararlar
- **Depolama**: Sabit slotlu tablo (CRASH64_MAX_DUMPS=16).
- **Format**: CRC32 ile korunan dump header + payload (register + stack bellek).
- **Reason siniflari**: 0=none, 1=PF, 2=GP, 3=INT, 4=OOM.

## Bilesenler
- `kernel/arch/x86_64/crash64.c` (host-test davranis modeli)
- `include/arch/x86_64/crash.h`

## Basari Kriterleri
- `make test-de64` 39A-J PASS.
