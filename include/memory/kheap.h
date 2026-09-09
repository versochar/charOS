#ifndef CHAROS_MEMORY_KHEAP_H
#define CHAROS_MEMORY_KHEAP_H

#include <stddef.h>
#include <stdint.h>

void kheap_init(void);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size, uint32_t align);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
void kheap_test(void);
void kheap_dump(void);

#endif