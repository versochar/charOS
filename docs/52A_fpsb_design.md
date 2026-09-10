# 52A - Flatpak Sandbox Design

## Amaç
Flatpak kum havuzu (`fpsb64`): app izin matrisi, firlatma kapisi, izin kisi / iptali.

## Kararlar
- **Izin bitleri**: NET, DBUS, FILESYS_HOST, GPU, X11, PULSE, DEVICES.
- Bilinmeyen/on tanimli bitler create'de -2.
- Ayni app id sadece bir kum havuzuna sahip olabilir.
- NET + DEVICES kombinasyonu guvensiz sayilir; firlatma -3 ve BLOCKED.
- Calisirken (RUNNING) izin ver/iptal yapilamaz (-3).

## Bilesenler
- `kernel/arch/x86_64/fpsb64.c`, `include/arch/x86_64/fpsb.h`
- Mevcut `flatpak64` (58H manifest) ile cakismaz.

## Ek duzeltme
- `test-pkg64` linkini kiran 41H taslaklari (`flatpak64_parse_ref`, `add_runtime`, `find_runtime`) gerceklendi; `test-pkg64` artik PASS.

## Basari Kriterleri
- `make test-de64` 52A-J PASS; `make test-pkg64` PASS.
