# 4A - Advanced Memory Management Tasarım

## Amaç
charOS için NUMA-aware alloc, 2MB/1GB huge page, KASAN gölge bellek ve swap-aware reclaim mimarisi.

## Kararlar
- **PMM uzantısı**: node-id etiketli buddy allocator, her NUMA node için ayrı free-list. `pmm64_alloc_node(node, order)` eklenir.
- **VMM uzantısı**: 4-level page table üzerinde 2MB (PS=1) ve 1GB (PDP PS=1) mapping. Büyük sayfa için `vmm64_map_huge()` API.
- **KASAN**: gölge bellek = RAM/8, `kasan64_poison()` / `kasan64_check()` ile alloc/free instrumentasyonu.
- **Reclaim**: düşük bellek eşiğinde LRU tabanlı sayfa geri alımı, swap-out yerine discard + ölçüm.

## Bileşenler
- `kernel/memory/pmm_node.c` (plan), `kernel/memory/vmm_huge.c` (plan), `kernel/memory/kasan.c` (plan)
- Mevcut `pmm.c`, `paging.c`, `kheap.c` ile uyumlu; mevcut API bozulmaz.

## Tehdit Modeli
- UAF / double-free: KASAN quarantine ile yakalanır.
- Yanlış node alloc: fallback Any-node + sayaç.

## Başarı Kriterleri
- Tasarım belgesi tamam, API taslağı hazır, mevcut `make test-de64` yeşil kalır.

## Sonraki Adım
4B API spesifikasyonu.
