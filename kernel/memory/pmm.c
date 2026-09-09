#include <memory/pmm.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <string.h>

extern uint32_t kernel_end; // linker.ld

static uint8_t pmm_bitmap[PMM_BITMAP_SIZE] __attribute__((aligned(4096)));
static uint16_t pmm_refcount[PMM_MAX_FRAMES];
static uint32_t pmm_free_frames = 0;
static uint32_t pmm_total_frames = 0;
static int pmm_ready = 0;

/* 11C: SMP güvenli frame alloc - test+set atomik olsun diye spinlock.
   İki CPU aynı frame'i ikinci kez alamaz (frame aliasing imkansız). */
static spinlock_t pmm_lock = SPINLOCK_INIT;

static inline void bitmap_set(uint32_t frame) {
    pmm_bitmap[frame / 8] |= (1 << (frame % 8));
}
static inline void bitmap_clear(uint32_t frame) {
    pmm_bitmap[frame / 8] &= ~(1 << (frame % 8));
}
static inline int bitmap_test(uint32_t frame) {
    return pmm_bitmap[frame / 8] & (1 << (frame % 8));
}

// Fiziksel adresi frame'e çevir, bitmap'te işaretle
static void pmm_reserve_region(uint32_t base, uint32_t length) {
    if (base >= PMM_MAX_MEMORY) return;
    if (base + length > PMM_MAX_MEMORY) length = PMM_MAX_MEMORY - base;
    uint32_t start_frame = base / PMM_PAGE_SIZE;
    uint32_t end_frame = (base + length + PMM_PAGE_SIZE - 1) / PMM_PAGE_SIZE;
    if (end_frame > PMM_MAX_FRAMES) end_frame = PMM_MAX_FRAMES;
    for (uint32_t i = start_frame; i < end_frame; i++) {
        if (!bitmap_test(i)) {
            // zaten free ise şimdi reserve et -> free count azalt
            bitmap_set(i);
            if (pmm_free_frames > 0) pmm_free_frames--;
        } else {
            bitmap_set(i);
        }
    }
}

/* 14G: ACPI tabloları gibi PMM havuzundaki RAM'i 1:1 map'lerken
 * çift tahsisi önlemek için herkese açık rezervasyon. */
void pmm_reserve(uint32_t base, uint32_t length) {
    pmm_reserve_region(base, length);
}

static void pmm_free_region(uint32_t base, uint32_t length) {
    if (base >= PMM_MAX_MEMORY) return;
    if (base + length > PMM_MAX_MEMORY) length = PMM_MAX_MEMORY - base;
    // 4K hizala
    uint32_t start = (base + PMM_PAGE_SIZE - 1) & ~(PMM_PAGE_SIZE - 1);
    uint32_t end = (base + length) & ~(PMM_PAGE_SIZE - 1);
    if (end <= start) return;
    if (start >= PMM_MAX_MEMORY) return;
    if (end > PMM_MAX_MEMORY) end = PMM_MAX_MEMORY;
    uint32_t start_frame = start / PMM_PAGE_SIZE;
    uint32_t end_frame = end / PMM_PAGE_SIZE;
    for (uint32_t i = start_frame; i < end_frame; i++) {
        if (bitmap_test(i)) {
            bitmap_clear(i);
            pmm_free_frames++;
        }
    }
}

void pmm_init(uint32_t mboot_ptr) {
    // Başlangıçta tüm 256MB'yi allocated işaretle
    memset(pmm_bitmap, 0xFF, PMM_BITMAP_SIZE);
    pmm_free_frames = 0;
    pmm_total_frames = PMM_MAX_FRAMES;
    pmm_ready = 0;

    vga_puts("[3B] PMM init mboot_ptr=0x"); vga_puthex(mboot_ptr); vga_puts("\n");
    serial_puts("[3B] PMM init mboot_ptr=0x"); serial_puthex(mboot_ptr); serial_puts("\n");

    struct multiboot_info* mbi = (struct multiboot_info*)mboot_ptr;
    if (mboot_ptr == 0 || !(mbi->flags & (1 << 6))) {
        vga_puts("[3B] WARN: no mmap, using mem_upper\n");
        serial_puts("[3B] WARN: no mmap\n");
        // Fallback: mem_upper kadar free yap (KB)
        uint32_t mem_kb = mbi ? mbi->mem_upper : 0;
        if (mem_kb > 0) {
            uint32_t total_bytes = (mem_kb + 1024) * 1024; // lower 1M + upper
            if (total_bytes > PMM_MAX_MEMORY) total_bytes = PMM_MAX_MEMORY;
            // 0-1M reserved, 1M-total free
            // Önce 1M sonrası free
            pmm_free_region(0x100000, total_bytes - 0x100000);
        } else {
            // Hiç bilgi yoksa 16MB varsay
            pmm_free_region(0x100000, 15*1024*1024);
        }
    } else {
        vga_puts("[3B] Parsing mmap...\n");
        serial_puts("[3B] Parsing mmap len="); serial_puthex(mbi->mmap_length); serial_puts(" addr=0x"); serial_puthex(mbi->mmap_addr); serial_puts("\n");
        uint32_t mmap_addr = mbi->mmap_addr;
        uint32_t mmap_length = mbi->mmap_length;
        for (uint32_t offset = 0; offset < mmap_length; ) {
            struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)(mmap_addr + offset);
            uint64_t base = entry->base_addr;
            uint64_t length = entry->length;
            uint32_t type = entry->type;

            vga_puts("  mmap base=0x"); vga_puthex((uint32_t)base);
            vga_puts(" len=0x"); vga_puthex((uint32_t)length);
            vga_puts(" type="); vga_putdec(type);
            vga_puts(type==1?" (avail)\n":" (reserved)\n");

            serial_puts("  mmap base=0x"); serial_puthex((uint32_t)base);
            serial_puts(" len=0x"); serial_puthex((uint32_t)length);
            serial_puts(" type="); serial_puthex(type); serial_puts("\n");

            if (type == 1) { // available
                if (base < PMM_MAX_MEMORY) {
                    uint32_t b = (uint32_t)base;
                    uint32_t l = (uint32_t)length;
                    if (b + l > PMM_MAX_MEMORY) l = PMM_MAX_MEMORY - b;
                    pmm_free_region(b, l);
                }
            }
            offset += entry->size + sizeof(entry->size);
        }
    }

    // Frame 0'ı her zaman reserved tut (null pointer yakalamak için)
    bitmap_set(0);
    if (!bitmap_test(0)) {
        bitmap_set(0);
        if (pmm_free_frames > 0) pmm_free_frames--;
    }
    // 13B: refcount'ları sıfırla, sonra allocated frame'ler için 1 yap
    memset(pmm_refcount, 0, sizeof(pmm_refcount));
    for (uint32_t i = 0; i < PMM_MAX_FRAMES; i++) if (bitmap_test(i)) pmm_refcount[i] = 1;

    // Kernel'i reserve et (1M - kernel_end)
    // 12E/pagefault kök nedeni: kheap (kernel_end sonrası, 8MB identity'e kadar)
    // PMM'de free görünüyordu; pmm_alloc_frame kheap struct'larını (örn main_task)
    // page table olarak verip memset ile siliyordu. 1M-8M arasını tümden reserve et.
    uint32_t kernel_start = 0x100000;
    uint32_t kernel_end_addr = (uint32_t)&kernel_end;
    // 4K hizala
    kernel_end_addr = (kernel_end_addr + 0xFFF) & ~0xFFF;
    uint32_t kernel_size = kernel_end_addr - kernel_start;
    vga_puts("[3B] Reserving kernel 0x"); vga_puthex(kernel_start);
    vga_puts(" - 0x"); vga_puthex(kernel_end_addr);
    vga_puts(" size "); vga_putdec(kernel_size/1024); vga_puts("KB\n");
    serial_puts("[3B] Reserving kernel size "); serial_puthex(kernel_size); serial_puts("\n");
    pmm_reserve_region(kernel_start, kernel_size);
    // kheap identity bölgesi (kernel_end - 8MB): kheap PMM'den bağımsız kullanır
    uint32_t kheap_top = 0x800000;
    if (kheap_top > kernel_end_addr) {
        serial_puts("[3B] Reserving kheap 0x"); serial_puthex(kernel_end_addr);
        serial_puts("-0x"); serial_puthex(kheap_top); serial_puts("\n");
        pmm_reserve_region(kernel_end_addr, kheap_top - kernel_end_addr);
    }

    // Bitmap'in kendisini de reserve et (eğer kernel dışında ise)
    // Bizim bitmap statik olarak kernel içinde (.bss) olduğu için zaten kernel reserve içinde

    // Toplam free hesapla
    pmm_total_frames = PMM_MAX_FRAMES;
    // pmm_free_frames zaten hesaplandı

    pmm_ready = 1;

    vga_puts("[3B] PMM total frames "); vga_putdec(pmm_total_frames);
    vga_puts(" free "); vga_putdec(pmm_free_frames);
    vga_puts(" ("); vga_putdec(pmm_free_frames*4); vga_puts(" KB)\n");
    serial_puts("[3B] PMM free "); serial_puthex(pmm_free_frames); serial_puts(" frames\n");
}

uint32_t pmm_alloc_frame(void) {
    if (!pmm_ready) return 0;
    uint32_t phys = 0;
    spin_lock(&pmm_lock);
    /* Alt 1MB (0-255 frame) legacy bölge - kernel identity-mapped ama CoW/user
       verisi için güvensiz (SMP trampoline 0x8000, BIOS data vb.). Kernel_end
       sonrasından almaya başla. */
    uint32_t min_frame = (0x100000 / PMM_PAGE_SIZE); /* 1MB */
    for (uint32_t i = min_frame; i < PMM_MAX_FRAMES; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            pmm_free_frames--;
            pmm_refcount[i] = 1;
            phys = i * PMM_PAGE_SIZE;
            break;
        }
    }
    spin_unlock(&pmm_lock);
    return phys;
}
void pmm_inc_ref(uint32_t phys) {
    if (phys % PMM_PAGE_SIZE) return;
    uint32_t f = phys / PMM_PAGE_SIZE;
    if (f >= PMM_MAX_FRAMES) return;
    spin_lock(&pmm_lock);
    if (bitmap_test(f) && pmm_refcount[f] < 0xFFFF) pmm_refcount[f]++;
    spin_unlock(&pmm_lock);
}
void pmm_dec_ref(uint32_t phys) {
    if (phys % PMM_PAGE_SIZE) return;
    uint32_t f = phys / PMM_PAGE_SIZE;
    if (f >= PMM_MAX_FRAMES) return;
    spin_lock(&pmm_lock);
    if (bitmap_test(f) && pmm_refcount[f] > 0) pmm_refcount[f]--;
    spin_unlock(&pmm_lock);
}
int pmm_get_ref(uint32_t phys) {
    if (phys % PMM_PAGE_SIZE) return -1;
    uint32_t f = phys / PMM_PAGE_SIZE;
    if (f >= PMM_MAX_FRAMES) return -1;
    return pmm_refcount[f];
}

void pmm_free_frame(uint32_t phys) {
    if (phys % PMM_PAGE_SIZE != 0) return;
    uint32_t frame = phys / PMM_PAGE_SIZE;
    if (frame >= PMM_MAX_FRAMES) return;
    if (frame == 0) return;
    spin_lock(&pmm_lock);
    if (!bitmap_test(frame)) {
        spin_unlock(&pmm_lock);
        vga_puts("[PMM] WARN double free frame 0x"); vga_puthex(phys); vga_puts("\n");
        return;
    }
    if (pmm_refcount[frame] > 1) {
        pmm_refcount[frame]--;
        spin_unlock(&pmm_lock);
        return;
    }
    // refcount 1 -> free
    pmm_refcount[frame] = 0;
    bitmap_clear(frame);
    pmm_free_frames++;
    spin_unlock(&pmm_lock);
}

uint32_t pmm_get_free_count(void) { return pmm_free_frames; }
uint32_t pmm_get_total_count(void) { return pmm_total_frames; }

void pmm_test(void) {
    vga_puts("[3B] PMM test alloc/free...\n");
    serial_puts("[3B] PMM test\n");
    uint32_t f1 = pmm_alloc_frame();
    uint32_t f2 = pmm_alloc_frame();
    uint32_t f3 = pmm_alloc_frame();
    vga_puts(" alloc f1=0x"); vga_puthex(f1); vga_puts(" f2=0x"); vga_puthex(f2); vga_puts(" f3=0x"); vga_puthex(f3); vga_puts("\n");
    serial_puts(" f1=0x"); serial_puthex(f1); serial_puts(" f2=0x"); serial_puthex(f2); serial_puts("\n");
    if (f1==0 || f2==0 || f3==0) {
        vga_puts("  FAIL OOM\n");
    } else {
        // Alias check: aynı frame iki kez gelmemeli
        if (f1==f2 || f2==f3 || f1==f3) {
            vga_puts("  FAIL alias!\n");
            serial_puts("FAIL alias\n");
        } else {
            vga_puts("  OK no alias\n");
        }
        pmm_free_frame(f2);
        uint32_t f4 = pmm_alloc_frame();
        vga_puts(" free f2, alloc f4=0x"); vga_puthex(f4); vga_puts(f4==f2?" (reuse OK)\n":" (reuse FAIL)\n");
        pmm_free_frame(f1); pmm_free_frame(f3); pmm_free_frame(f4);
        vga_puts(" free all, free count "); vga_putdec(pmm_get_free_count()); vga_puts("\n");
    }
}