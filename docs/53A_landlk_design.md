# 53A - Seccomp & Landlock Design

## Amaç
Patika tabanlı LSM (Landlock benzeri, `landlk64`) + mevcut seccomp kural kümelerinin tamamlanması.

## Kararlar
- **Ruleset**: handled_access bitleri (EXEC/WRITE/READ); bilinmeyen bit -2.
- **Kural**: (ruleset, patika alt ağacı, izin bitleri); handled dışı izin -3.
- **restrict_self**: ruleset aktifleşir, salt-okunur olur (yeni kural -2).
- **check**: aktif ruleset yalnızca kuralin kapsadığı erisimi verir; "/" ile biten patika alt ağacı kucaklar.
- En son restrict edilen ruleset aktif katmandır.

## Düzeltmeler
- 37D seccomp stubs (`add`, `strict`, `count`) gerçeklendi; `test-sec64` link/FAIL düzeltildi.
- 37G AppArmor stubs (`add_profile`, `add_rule`, `check`) gerçeklendi.

## Basari Kriterleri
- `make test-de64` 53A-J PASS; `make test-sec64` PASS.
