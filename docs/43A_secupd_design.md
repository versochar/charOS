# 43A - Secure Update Signing Design

## Amaç
Guncellemeleri imzala ve dogrula; politika katmani `secupd64` ile imza yoksa/bozuksa apply engelle.

## Kararlar
- **Anahtar tablosu**: id -> secret (gizli). Cakisan id reddedilir.
- **MAC**: ad + surum + icerik ozeti (payload_hash) gizli anahtarla FNV-1a turevli karistirma.
- **Politikalar**: RELAXED (imza yoksa kabul), ENFORCED (imzali dogrulanmis kayit sarti), STRICT (yalnizca kayitli anahtar).
- Stage aninda imza dogrulanir; hatali imza -4 ile reddedilir.

## Bilesenler
- `kernel/arch/x86_64/secupd64.c`, `include/arch/x86_64/secupd.h`
- `sign64` (41D primitif) ile mustakil; politika semantigi ayri.

## Basari Kriterleri
- `make test-de64` 43A-J PASS.
