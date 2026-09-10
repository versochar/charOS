# 51A - Container Runtime Design

## Amaç
Konteyner calisma zamani (`ctr64`): yasam dongusu (create/start/pause/resume/stop), PID atama, bellek siniri.

## Kararlar
- **Slot tablosu**: name + image + mem_limit + pid + id + state.
- **Yasam dongusu**: CREATED -> RUNNING <-> PAUSED -> STOPPED.
- Ayni isimli ikinci konteyner -3; sifir bellek siniri -2.
- Id ve PID benzersiz; hakla artti.

## Bilesenler
- `kernel/arch/x86_64/ctr64.c`, `include/arch/x86_64/ctr.h`
- Mevcut `runtime64` (58I wizard) ile cakismaz.

## Basari Kriterleri
- `make test-de64` 51A-J PASS.
