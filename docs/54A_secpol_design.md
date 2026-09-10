# 54A - AppArmor/SELinux Policy Design

## Amaç
AppArmor (yol tabanlı) ile SELinux (bağlam tabanlı) izin mantığını birleştiren politika motoru (`secpol64`).

## Kararlar
- **Profil**: ad + yol öneki + mode (enforce/complain).
- **Kural**: profilde patika + op bitleri (READ/WRITE/EXEC) + selinux bayrağı.
- Kural adayları profilden bağımsız saklanır; `rule_id` döner.
- Mode ENFORCE: önek içinde kural yoksa red. COMPLAIN: kural yoksa izin.
- Önek dışındaki patikalarda default-allow (profil yalnızca kendi ağacını korur).
- Mevcut 37F/38G/37G primitiflerine dokunmaz.

## Basari Kriterleri
- `make test-de64` 54A-J PASS.
