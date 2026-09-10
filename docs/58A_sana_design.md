# 58A - Static Analysis Design

## Amaç
Sabit kod analizi çerçevesi (`sana64`): null deref, buffer overflow, uninit use vb. kural tabanlı tarama.

## Kararlar
- Kural kaydı: id, severity, açıklama.
- Basıt tarama: etkin kurallar sayılır; gerçek derleyici analizi yerine davranış modeli.
- Rapor formatı: `dosya:satir:kural`.
- Buffer tarama: 0x00/0xFF bayt sayımı.
