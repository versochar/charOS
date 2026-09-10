# 57A - Fuzzing Infrastructure Design

## Amaç
AFL benzeri mutasyoncu fuzzer cekirdegi (`fuzz64`): korpus, xorshift PRNG, mutasyon ve crash depolama.

## Kararlar
- **PRNG**: xorshift32; `xstart` ile tohumlanir.
- **Korpus**: 32 tohum, 1024B cap; cagri sayisi ile yol cesitliligi.
- **iterasyon**: rastgele tohum sec + kopyala + buyut/carpik/rastgele boz.
- **mutate**: tek tek bayt operasyonlari (rastgele, bit flip, sifirla, doyur, takas) + buyutme.
- **crash**: neden ve uzunluk karistirilarak saklanir.
- `corpus_add_file` kernel ortaminda stub (-2).

## Basari Kriterleri
- `make test-de64` 57A-J PASS.
