#include <memory/kheap.h>
#include <memory/paging.h>
#include <memory/pmm.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

extern uint32_t kernel_end;

#define KHEAP_INITIAL_SIZE (1*1024*1024) // 1MB
#define KHEAP_MAGIC 0x12345678

struct block_header {
    uint32_t magic;
    size_t size; // usable size after header
    int free;
    struct block_header* next;
    struct block_header* prev;
} __attribute__((packed));

static uint8_t* heap_start = 0;
static uint8_t* heap_end = 0;
static uint8_t* heap_max = 0; // 8MB identity limit for now
static struct block_header* head = 0;
static spinlock_t kheap_lock = SPINLOCK_INIT;

static void kheap_expand(size_t needed) {
    // Needed size + header, page aligned
    size_t to_alloc = (needed + sizeof(struct block_header) + 0xFFF) & ~0xFFF;
    // En basit: static heap içindeyiz (0-8MB identity), sadece heap_end'i ilerlet
    // PMM'den frame alıp paging_map ile yeni sayfaları map etmemiz gerekir eğer heap_max'a ulaştıysak
    // Şimdilik 0-8MB identity olduğu için sadece heap_end kontrolü yeterli
    if (heap_end + to_alloc > heap_max) {
        // Yeni frame'ler allocate edip map'le (8MB sonrası için)
        // 8MB = 0x800000, sonrası için paging_map gerekir
        uint32_t needed_pages = (to_alloc + 0xFFF) / 0x1000;
        for (uint32_t i = 0; i < needed_pages; i++) {
            uint32_t phys = pmm_alloc_frame();
            if (!phys) {
                vga_puts("[KHEAP] OOM expand\n");
                serial_puts("[KHEAP] OOM expand\n");
                return;
            }
            uint32_t virt = (uint32_t)heap_end + i*0x1000;
            // Eğer virt zaten 0-8MB içindeyse zaten mapped, tekrar map'e gerek yok
            if (virt >= 0x800000) {
                paging_map(virt, phys, PAGE_PRESENT | PAGE_RW);
            } else {
                // Identity bölgesinde phys frame'i zaten kullanımda sayılır ama pmm'den aldık
                // Bu frame'i heap için kullanacağız, identity map zaten var ama phys farklı
                // Aslında 0-8MB identity map'te her virt == phys, ama yeni phys'i aynı virt'te map'lemek için
                // page table entry'yi overwrite etmemiz gerekir
                paging_map(virt, phys, PAGE_PRESENT | PAGE_RW);
            }
        }
        heap_max = heap_end + to_alloc;
        if (heap_max > (uint8_t*)0x800000) heap_max = (uint8_t*)0x800000; // şimdilik 8MB limit
    }
    // Yeni block oluştur heap_end'de
    struct block_header* new_block = (struct block_header*)heap_end;
    new_block->magic = KHEAP_MAGIC;
    new_block->size = to_alloc - sizeof(struct block_header);
    new_block->free = 1;
    new_block->next = 0;
    new_block->prev = 0;
    // Listeye ekle
    if (!head) {
        head = new_block;
    } else {
        struct block_header* cur = head;
        while(cur->next) cur = cur->next;
        cur->next = new_block;
        new_block->prev = cur;
    }
    heap_end += to_alloc;
}

void kheap_init(void) {
    uint32_t k_end = ((uint32_t)&kernel_end + 0xFFF) & ~0xFFF;
    // 1MB hizala ve 2MB'dan başlat ki kernel ile çakışmasın ve page aligned olsun
    // kernel_end ~0x111000, heap_start 0x112000 olabilir ama 0x200000 daha güvenli (hole)
    // Spec'de heap için ayrı alan - biz kernel_end sonrası kullanıyoruz
    heap_start = (uint8_t*)k_end;
    // 4K hizalı olduğundan emin ol
    heap_max = (uint8_t*)0x800000; // 8MB identity sınırı
    heap_end = heap_start + KHEAP_INITIAL_SIZE;
    if (heap_end > heap_max) heap_end = heap_max;

    head = (struct block_header*)heap_start;
    head->magic = KHEAP_MAGIC;
    head->size = (heap_end - heap_start) - sizeof(struct block_header);
    head->free = 1;
    head->next = 0;
    head->prev = 0;

    vga_puts("[3C] KHeap init start=0x"); vga_puthex((uint32_t)heap_start);
    vga_puts(" end=0x"); vga_puthex((uint32_t)heap_end);
    vga_puts(" size "); vga_putdec((heap_end-heap_start)/1024); vga_puts("KB\n");
    serial_puts("[3C] KHeap init start=0x"); serial_puthex((uint32_t)heap_start);
    serial_puts(" size "); serial_puthex(heap_end-heap_start); serial_puts("\n");
}

static struct block_header* find_free(size_t size) {
    struct block_header* cur = head;
    while(cur) {
        if(cur->free && cur->size >= size) return cur;
        cur = cur->next;
    }
    return 0;
}

static void split_block(struct block_header* block, size_t size) {
    // block size >= size + header + 16 minimal split
    if (block->size >= size + sizeof(struct block_header) + 16) {
        struct block_header* new_block = (struct block_header*)((uint8_t*)block + sizeof(struct block_header) + size);
        new_block->magic = KHEAP_MAGIC;
        new_block->size = block->size - size - sizeof(struct block_header);
        new_block->free = 1;
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) block->next->prev = new_block;
        block->next = new_block;
        block->size = size;
    }
}

static void kheap_check_integrity(const char* tag) {
    // 13F tanı: heap listesi bozuldu mu? (double-alloc ezilme tespiti)
    struct block_header* cur = head;
    struct block_header* prev = 0;
    int n = 0;
    while (cur && n < 64) {
        if (cur->magic != KHEAP_MAGIC) {
            extern void serial_puts(const char* s);
            extern void serial_puthex(uint32_t v);
            vga_puts("[KHEAP] CORRUPT at "); vga_puthex((uint32_t)cur);
            vga_puts(" tag="); vga_puts(tag); vga_puts("\n");
            serial_puts("[KHEAP] CORRUPT "); serial_puthex((uint32_t)cur);
            serial_puts(" tag="); serial_puts(tag);
            serial_puts(" magic="); serial_puthex(cur->magic);
            serial_puts("\n");
            return;
        }
        if (prev && cur != (struct block_header*)((uint8_t*)prev + sizeof(struct block_header) + prev->size)) {
            extern void serial_puts(const char* s);
            extern void serial_puthex(uint32_t v);
            serial_puts("[KHEAP] OVERLAP prev="); serial_puthex((uint32_t)prev);
            serial_puts(" cur="); serial_puthex((uint32_t)cur);
            serial_puts(" tag="); serial_puts(tag); serial_puts("\n");
        }
        prev = cur; cur = cur->next; n++;
    }
}

void* kmalloc(size_t size) {
    if (size == 0) return 0;
    // 8-byte hizala
    size = (size + 7) & ~7;

    uint32_t flags;
    spin_lock_irqsave(&kheap_lock, &flags);

    kheap_check_integrity("kmalloc_entry");

    struct block_header* block = find_free(size);
    if (!block) {
        // Expand heap
        kheap_expand(size);
        block = find_free(size);
        if (!block) {
            spin_unlock_irqrestore(&kheap_lock, flags);
            vga_puts("[KHEAP] kmalloc OOM size "); vga_putdec(size); vga_puts("\n");
            serial_puts("[KHEAP] OOM "); serial_puthex(size); serial_puts("\n");
            return 0;
        }
    }

    split_block(block, size);
    block->free = 0;
    block->magic = KHEAP_MAGIC;

    void* ptr = (uint8_t*)block + sizeof(struct block_header);

    kheap_check_integrity("kmalloc_exit");

    spin_unlock_irqrestore(&kheap_lock, flags);
    return ptr;
}

void kfree(void* ptr) {
    if (!ptr) return;
    uint32_t flags;
    spin_lock_irqsave(&kheap_lock, &flags);

    struct block_header* block = (struct block_header*)((uint8_t*)ptr - sizeof(struct block_header));
    if (block->magic != KHEAP_MAGIC) {
        vga_puts("[KHEAP] kfree invalid magic ptr=0x"); vga_puthex((uint32_t)ptr); vga_puts("\n");
        serial_puts("[KHEAP] invalid free\n");
        spin_unlock_irqrestore(&kheap_lock, flags);
        return;
    }
    block->free = 1;
    // Coalesce next
    if (block->next && block->next->free && block->next->magic==KHEAP_MAGIC) {
        block->size += sizeof(struct block_header) + block->next->size;
        block->next = block->next->next;
        if (block->next) block->next->prev = block;
    }
    // Coalesce prev
    if (block->prev && block->prev->free && block->prev->magic==KHEAP_MAGIC) {
        block->prev->size += sizeof(struct block_header) + block->size;
        block->prev->next = block->next;
        if (block->next) block->next->prev = block->prev;
        // block artık prev'in içinde
    }

    spin_unlock_irqrestore(&kheap_lock, flags);
}

void* krealloc(void* ptr, size_t size) {
    if (!ptr) return kmalloc(size);
    if (size==0){ kfree(ptr); return 0; }
    struct block_header* block = (struct block_header*)((uint8_t*)ptr - sizeof(struct block_header));
    size_t old_size = block->size;
    if (size <= old_size) return ptr;
    void* new_ptr = kmalloc(size);
    if (!new_ptr) return 0;
    memcpy(new_ptr, ptr, old_size);
    kfree(ptr);
    return new_ptr;
}

void* kmalloc_aligned(size_t size, uint32_t align) {
    // Align must be power of 2
    // Allocate extra + align, then align pointer and store original
    // Simplest: allocate size + align + header, then align
    // For 3C, sadece page align (4096) için kullanılacak, o yüzden özel yol
    if (align <= 8) return kmalloc(size);
    void* raw = kmalloc(size + align + sizeof(void*));
    if (!raw) return 0;
    uintptr_t raw_addr = (uintptr_t)raw;
    uintptr_t aligned = (raw_addr + sizeof(void*) + align - 1) & ~(align - 1);
    void** store = (void**)(aligned - sizeof(void*));
    *store = raw;
    return (void*)aligned;
    // Not: kfree_aligned için wrapper gerekir ama şimdilik kullanılmıyor
}

void kheap_test(void) {
    vga_puts("[3C] KHeap test...\n");
    serial_puts("[3C] KHeap test\n");
    void* a = kmalloc(16);
    void* b = kmalloc(32);
    void* c = kmalloc(64);
    vga_puts(" a=0x"); vga_puthex((uint32_t)a);
    vga_puts(" b=0x"); vga_puthex((uint32_t)b);
    vga_puts(" c=0x"); vga_puthex((uint32_t)c); vga_puts("\n");
    serial_puts(" a=0x"); serial_puthex((uint32_t)a); serial_puts("\n");
    if (!a||!b||!c) { vga_puts(" FAIL alloc\n"); return; }
    // Write test
    memset(a, 0xAA, 16); memset(b, 0xBB, 32); memset(c, 0xCC, 64);
    kfree(b);
    void* d = kmalloc(16);
    vga_puts(" free b, alloc d=0x"); vga_puthex((uint32_t)d);
    vga_puts(d==b?" (reuse OK)\n":" (reuse maybe)\n");
    kfree(a); kfree(c); kfree(d);
    // Coalesce test: allocate large
    void* large = kmalloc(512);
    vga_puts(" large=0x"); vga_puthex((uint32_t)large); vga_puts(large?" OK\n":" FAIL\n");
    if (large) kfree(large);
    vga_puts("[3C] KHeap test done\n");
    serial_puts("[3C] test done\n");
}

void kheap_dump(void) {
    vga_puts("--- KHeap dump ---\n");
    struct block_header* cur = head;
    int idx=0;
    while(cur) {
        vga_puts(" "); vga_putdec(idx++); vga_puts(": 0x"); vga_puthex((uint32_t)cur);
        vga_puts(" size "); vga_putdec(cur->size);
        vga_puts(cur->free?" FREE":" USED");
        vga_puts("\n");
        cur = cur->next;
        if(idx>20){ vga_puts(" ...\n"); break; }
    }
}