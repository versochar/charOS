# 4D - Advanced Memory Kod Geliştirme

## Geliştirmeler
- `advmem64.c`: node alloc (node sayaçlı), huge-map hizalama denetimi (2MB/1GB), KASAN poison bitmap, reclaim sayacı.
- Sabit zamanlı olmayan kritik yol yok; tüm hata yolları negatif testli.
- Gerçek PMM entegrasyonu için not: buddy order maskesi korunmalı, huge mappingte TLB shootdown (`invlpg`) gerekir.

## Davranış Modeli
- `advmem64_alloc_node`: node>=0 sayacı artırır, phys = 0x100000 + sayaç*0x1000 döner.
- `advmem64_map_huge`: virt/phys hizalaması yanlışsa -1, level 1/2 dışında -1.
- `kasan` modeli: poison bölgesi işaretlenir, check ihlalde -1.
- `reclaim`: çağrı sayacı kadar freed bildirir.

## Test
4D2-4D4 host testleri PASS.

## Sonraki Adım
4E unit test yazma.
