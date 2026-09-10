# 56A - Hardening & Mitigations Design

## Amaç
X86 acik modeli (`mitig64`): spectre-v1/v2, meltdown, retbleed, storebleed, mds, l1tf aciklari icin azaltim teknikleri ve guven denetimi.

## Kararlar
- **Acik tablosu**: 7 bilinen x86 acigi; her biri zorunlu teknik kumesine sahiptir.
- **Uygulama**: teknik bitleri birikir; zorunlu kume tamamlaninca MITIGATED, eksikse PARTIAL.
- **auto_verify**: bildirilen teknikler tum kapali aciklari kapatmali (tam dogrulama).
- **cpu_trustworthy**: kapali acik kalmadi ve auto_verify gecerse 1.
- `MITIG64_TECH_*` makrolari longmode.h'de ortak; `MITIG64_SRDS` vb. dumy kisa ad kullanilmaz.

## Basari Kriterleri
- `make test-de64` 56A-J PASS.
