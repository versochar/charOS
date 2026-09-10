# 3A - Memory Management Core - Tasarım

## Amaç
charOS için fiziksel ve sanal bellek yönetim mimarisi

## Bileşenler
- pmm64: Physical Memory Manager
- vmm64: Virtual Memory Manager
- kheap64: Kernel heap

## Tasarım Kararları
- Buddy allocator
- Slab allocator
- Page table 4-level

## Test
Geçerli
