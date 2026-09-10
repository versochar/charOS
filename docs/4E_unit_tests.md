# 4E - Advanced Memory Unit Testleri

## Kapsam
- Node alloc parametre hataları (NULL out, negatif order).
- Huge-map hizalama hataları (hizasız virt/phys, geçersiz level).
- KASAN poison/check tutarlılığı.
- Reclaim NULL ve sayaç davranışı.

## Senaryolar
- 4E2 init, 4E3 alloc param, 4E4 huge hizalama, 4E5 kasan, 4E6 reclaim.

## Kapsama
Tüm yeni API hata yolları test edildi.

## Sonuç
PASS.
