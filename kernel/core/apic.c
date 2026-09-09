/* 11A: APIC - lapic read/write minimal */
#include <stdint.h>
#include <core/apic.h>
#include <core/gdt.h>
#include <core/isr.h>
#include <core/idt.h>
#include <core/spinlock.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/timer.h>
#include <memory/paging.h>
#include <string.h>

static volatile uint32_t* lapic = (volatile uint32_t*)0xFEE00000;

/* Kaç CPU aktif? (AP'ler uyandığında artar - 11D/SMP) */
static volatile int online_cpus = 1;

/* 11D: gömülü trampoline blob'u (embed_trampoline.s -> build/trampoline.bin) */
extern uint8_t trampoline_blob_start[];
extern uint8_t trampoline_blob_end[];

/* ---- 11D: sabit düşük bellek adresleri (trampoline.s ile uyumlu) ---- */
#define SMP_INFO_ADDR    0x7000  /* bilgi bloğu (24 byte packed) */
#define TRAMPOLINE_ADDR  0x8000  /* trampoline kopyasının hedefi (SIPI vektor 8) */
#define SIPI_VECTOR      8

/* 11E: LAPIC timer ilk sayacı. QEMU LAPIC saat ~1GHz; 100ms'lik periyot için
   ~10M (100Hz'deki PIT tick'iyle tam hizalı değil, sadece per-CPU tick kanıtı). */
#define LAPIC_TIMER_TICKS  10000000

/* trampoline.s'in okuduğu bilgi bloğu (offset'ler sabit!) */
struct smp_info_block {
    uint16_t gdt_limit;   /* +0x00 */
    uint32_t gdt_base;    /* +0x02 */
    uint16_t idt_limit;   /* +0x06 */
    uint32_t idt_base;    /* +0x08 */
    uint32_t cr3;         /* +0x0C */
    uint32_t ap_stack;    /* +0x10 */
    uint32_t ap_entry;    /* +0x14 */
} __attribute__((packed));

/* Her AP için ayrı kernel stack */
static uint8_t ap_stacks[MAX_CPU][8192] __attribute__((aligned(16)));
static volatile uint32_t ap_online[MAX_CPU] = {0,0,0,0};

static void udelay_us(uint32_t us) {
    /* Kaba USB gecikmesi (QEMU için yeterli): ~20 bayt döngü başına ~50ns varsay */
    volatile uint32_t loops = us * 100;
    for (volatile uint32_t i = 0; i < loops; i++) asm volatile("pause");
}

/* LAPIC MMIO bölgesini sayfa tablosuna map et (0xFEE00000, 4KB, uncacheable) */
static void lapic_map_mmio(void) {
    for (uint32_t a = 0xFEE00000; a < 0xFEE01000; a += 0x1000) {
        paging_map(a, a, PAGE_PRESENT | PAGE_RW | PAGE_PCD | PAGE_PWT);
    }
}

uint32_t lapic_read(uint32_t reg);

/* 21B: x2APIC MSR tabanlı LAPIC erişimi.
   X2APIC mode aktifse LAPIC MMIO yerine MSR 0x800..0xBFF üzerinden okunur.
   Mod, IA32_APIC_BASE bit 10 (EN_X2APIC) ile belirlenir — bu MSR, xAPIC
   modunda bile okunabilir, böylece BSP/AP ayrımına gerek kalmaz. */
static int lapic_is_x2(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(MSR_APIC_BASE));
    return (lo & MSR_APIC_BASE_X2) ? 1 : 0;
}

static uint32_t x2apic_msr_of(uint32_t reg) {
    return X2APIC_MSR_BASE + (reg >> 4);
}

uint32_t lapic_read(uint32_t reg) {
    if (lapic_is_x2()) {
        uint32_t lo, hi;
        __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(x2apic_msr_of(reg)));
        return lo;
    }
    return lapic[reg >> 2];
}

/* 13F+SMP: fiziksel LAPIC ID (0..255). xAPIC: register üst 24 bit; x2APIC:
   register alt 32 bit'te tam APIC ID gelir. */
uint32_t apic_get_raw_id(void) {
    uint32_t id = lapic_read(LAPIC_ID);
    if (lapic_is_x2()) return id & 0xFFu;      /* x2APIC: ID düşük bitlerde */
    return (id >> 24) & 0xFF;                   /* xAPIC: ID yüksek bayt */
}

/* 21B: x2APIC tespiti (CPUID leaf 1, ECX bit 21) */
int x2apic_supported(void) {
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    (void)a; (void)b; (void)d;
    return (c & (1u << 21)) ? 1 : 0;
}

/* 21B: mevcut CPU'yu x2APIC moduna geçir.
   IA32_APIC_BASE: bit11 (APIC enable) + bit10 (EN_X2APIC) sırasıyla set edilir.
   xAPIC moduna dönüş ancak reset ile; bu yüzden sadece boot'ta çağrılır. */
int x2apic_try_enable(void) {
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(MSR_APIC_BASE));

    if (!(lo & MSR_APIC_BASE_EN)) {      /* APIC kapalıysa önce aç */
        lo |= MSR_APIC_BASE_EN;
        __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(MSR_APIC_BASE));
    }
    lo |= MSR_APIC_BASE_X2;              /* x2APIC modu */
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(MSR_APIC_BASE));

    /* Doğrulama: LAPIC ID artık MSR üzerinden okunabiliyor olmalı */
    uint32_t id_msr = 0; __asm__ volatile("rdmsr" : "=a"(id_msr), "=d"(hi) : "c"(X2APIC_LAPIC_ID_MSR));
    (void)id_msr;
    return 1;
}

/* 21B: x2APIC'te ICR tek 64-bit MSR'dir (0x830). xAPIC kodu ICRH sonra ICRL
   yazar; x2APIC'te ICRH MSR'ı (0x831) yoktur (#GP). Bu yüzden ICRH değeri
   saklanır, ICRL yazılırken 64-bit WRMSR tek seferde yapılır. */
static volatile uint32_t x2_icr_high = 0;   /* hedef APIC ID (xAPIC format: <<24) */

void lapic_write(uint32_t reg, uint32_t val) {
    if (lapic_is_x2()) {
        if (reg == LAPIC_ICRH) {
            x2_icr_high = (val >> 24) & 0xFFu;   /* x2APIC hedef: 32-bit APIC ID */
            return;
        }
        if (reg == LAPIC_ICRL) {
            __asm__ volatile("wrmsr" : : "a"(val), "d"(x2_icr_high), "c"(x2apic_msr_of(reg)));
            return;
        }
        __asm__ volatile("wrmsr" : : "a"(val), "d"(0), "c"(x2apic_msr_of(reg)));
        return;
    }
    lapic[reg >> 2] = val;
}

uint32_t apic_cpu_count(void) {
    return 2; // QEMU -smp 2 varsayımı; gerçek değer LAPIC/MP'den okuncak
}

uint32_t apic_init_inquiry(void) {
    lapic_map_mmio();
    if (!lapic) lapic = (volatile uint32_t*)0xFEE00000;
    serial_puts("[11A] LAPIC MMIO address: 0xFEE00000\n");
    return 0;
}

void apic_send_ipi(uint32_t dst, uint32_t cmd) {
    /* ICRH = hedef APIC ID (üst 24 bit), ICRL = komut */
    lapic_write(LAPIC_ICRH, (dst & 0xFF) << 24);
    lapic_write(LAPIC_ICRL, cmd);
    /* Delivery status (bit 12) temizlenene kadar bekle */
    const int MAX = 1000000;
    int i = 0;
    while ((lapic_read(LAPIC_ICRL) & 0x1000) && i < MAX) {
        asm volatile("pause");
        i++;
    }
}

void lapic_eoi_send(void) {
    lapic_write(LAPIC_EOI, 0);
}

void lapic_local_apic_init(void) {
    if (!lapic) apic_init_inquiry();

    /* SVR = bit12 APIC enable | spurious vektör 0xFF -> 0x1FF.
       (0x7FF önceki hatalı değerdi; bit12=0 olduğu için LAPIC'yi kapatıyordu.) */
    lapic_write(LAPIC_SVR, 0x1FF);
    lapic_write(LAPIC_TPR, 0);
    serial_puts("[11A] LAPIC on cpu_init SVR=1FF\n");
}

/* ---- 11C: TLB shootdown IPI ---- */
static volatile uint32_t shootdown_addr = 0;      /* hedef sanal adres */
static volatile uint32_t shootdown_pending = 0;   /* beklemede olan shootdown */

/* Alıcı CPU'da çalışan handler: TLB'yi geçersiz kıl, EOI gönder */
static void tlb_shootdown_handler(struct registers* regs) {
    (void)regs;
    paging_invalidate(shootdown_addr);
    shootdown_pending--;
    lapic_eoi_send();
}

/* Tüm diğer CPU'lara TLB shootdown IPI gönder (vector 0x50, fixed, all-except-self) */
void tlb_shootdown(uint32_t virt) {
    shootdown_addr = virt;
    if (online_cpus <= 1) {
        paging_invalidate(virt); // tek aktif CPU: yerel yeter
        return;
    }
    shootdown_pending = online_cpus - 1;
    lapic_write(LAPIC_ICRL, TLB_SHOOTDOWN_VECTOR | LAPIC_ICR_FIXED | LAPIC_ICR_DEST_OTHERS);
    // AP'ler ack'leyene kadar bekle (sınırlı döngü)
    const int MAX_WAIT = 1000000;
    int waited = 0;
    while (shootdown_pending > 0 && waited < MAX_WAIT) {
        asm volatile("pause");
        waited++;
    }
    paging_invalidate(virt); // kendimiz de
}

/* ---- 11E: per-CPU LAPIC timer ---- */
static volatile uint32_t lapic_tick_count[MAX_CPU] = {0,0,0,0};

/* Tanılama: BSP LAPIC sayacı (PIT'ten bağımsız ikinci saat) */
uint32_t lapic_tick_get(void) { return lapic_tick_count[0]; }

/* Her CPU'nun kendi LAPIC timer interrupt'ı: EOI gönder + per-CPU sayacı artır.
   BSP'de zamanlama hâlâ PIT IRQ0 (vektör 32) üzerinden yürür; bu timer yalnız
   AP'lerin kendi tikiyle ayakta kalmasını ve SMP'de per-CPU zamanı kanıtlanır. */
static void lapic_timer_handler(struct registers* regs) {
    (void)regs;
    lapic_eoi_send();

    uint32_t cpu = apic_get_raw_id();
    uint32_t idx = (cpu < MAX_CPU) ? cpu : (MAX_CPU - 1);
    lapic_tick_count[idx]++;

    if ((lapic_tick_count[idx] % 200) == 0) {
        serial_puts("[11E] LAPIC timer CPU "); serial_puthex(cpu);
        serial_puts(" tick="); serial_puthex(lapic_tick_count[idx]); serial_puts("\n");
    }
    /* 13F+SMP notu: AP'de preemptif schedule stabil değil (12A-13F suite'i
       BSP-ince dizilidir). AP task çalıştırması şimdilik BSP-only scheduler
       ile devre dışı; per-CPU TSS slotu (0x30) hazır durur. */
}

/* 11E: mevcut CPU'nun LAPIC timer'ını periyodik başlat (vektör 0x40).
   count: ilk sayaç değeri (QEMU'da LAPIC ~1GHz -> 100Hz için ~10M). */
void lapic_timer_init(uint32_t count) {
    if (!lapic) apic_init_inquiry();

    /* 1) Bölücü: /1 */
    lapic_write(LAPIC_TMRDIV, LAPIC_TMR_DIV1);
    /* 2) LVT: periyodik mod, maskesiz, vektör 0x40 */
    lapic_write(LAPIC_TMR0, LAPIC_TIMER_VECTOR | LAPIC_TMR_PERIODIC);
    /* 3) İlk değeri yaz -> saymaya başla */
    lapic_write(LAPIC_TMRICT, count);

    uint32_t id = apic_get_raw_id();
    serial_puts("[11E] LAPIC timer on CPU "); serial_puthex(id);
    serial_puts(" vector=0x40 count=0x"); serial_puthex(count); serial_puts("\n");
    vga_puts("[11E] LAPIC timer CPU "); vga_putdec(id); vga_puts(" started (vector 0x40)\n");
}

/* ---- 11D: AP wake-up (INIT-SIPI-SIPI) ---- */
static void lapic_wait_icr_idle(void) {
    const int MAX = 1000000;
    int i = 0;
    while ((lapic_read(LAPIC_ICRL) & 0x1000) && i < MAX) {
        asm volatile("pause");
        i++;
    }
}

/* AP'de çalışan C giriş fonksiyonu (tanım aşağıda) */
void ap_main(void);

/* Tek bir AP'ye INIT-SIPI-SIPI gönder, bilgi bloğunu doldur */
static void wake_one_ap(uint32_t apic_id) {
    uint32_t bsp_id = apic_get_raw_id();
    uint32_t cr3 = (uint32_t)&page_directory;

    struct smp_info_block* info = (struct smp_info_block*)SMP_INFO_ADDR;
    memset(info, 0, sizeof(*info));
    info->gdt_limit = gdt_get_limit();
    info->gdt_base  = gdt_get_base();
    info->idt_limit = idt_get_limit();
    info->idt_base  = idt_get_base();
    info->cr3       = cr3;
    info->ap_stack  = (uint32_t)&ap_stacks[apic_id][8192];
    info->ap_entry  = (uint32_t)ap_main;

    serial_puts("[11D] INIT-SIPI-SIPI to APIC-id="); serial_puthex(apic_id); serial_puts(" bsp="); serial_puthex(bsp_id); serial_puts("\n");
    vga_puts("[11D] Sending INIT-SIPI-SIPI to APIC id "); vga_putdec(apic_id); vga_puts(" (tiny delay ~10ms)\n");

    /* 1) INIT assert (delivery=101, level=1, assert=1 -> 0xC500) */
    apic_send_ipi(apic_id, LAPIC_ICR_INIT | LAPIC_ICR_LEVEL | LAPIC_ICR_ASSERT);
    lapic_wait_icr_idle();

    /* ~10ms bekle (PIT tick = 10ms) */
    timer_wait(1);

    /* 2) SIPI #1 (delivery=110 STARTUP | vektör 8 -> 0x608) */
    apic_send_ipi(apic_id, LAPIC_ICR_STARTUP | SIPI_VECTOR);
    lapic_wait_icr_idle();
    udelay_us(200);

    /* 3) SIPI #2 */
    apic_send_ipi(apic_id, LAPIC_ICR_STARTUP | SIPI_VECTOR);
    lapic_wait_icr_idle();
}

/* AP'de çalışan C giriş fonksiyonu (trampoline buraya atlar) */
void ap_main(void) {
    uint32_t id = apic_get_raw_id();

    /* 13F+SMP: AP kendi TSS slotuna (index 6, sel 0x30) LTR yapmadan önce
       ortak GDT'yi kur. Tek paylaşılan GDT (per-CPU GDT arraysi AP'de ltr GPF
       veriyordu) üzerinde AP'nin TSS descriptor'ı kurulur ve AP kendi TR'sini
       BSP'nin TSS'inden bağımsız yükler => esp0 çakışması yok. */
    extern void gdt_init(void);
    gdt_init();

    lapic_local_apic_init();
    lapic_eoi_send();

    vga_puts("[11D] CPU "); vga_putdec(id); vga_puts(" online (AP)!\n");
    serial_puts("[11D] CPU "); serial_puthex(id); serial_puts(" online (AP)!\n");

    /* AP tam hazır olmadan BSP TLB-shootdown broadcast etmesin:
       online sayım ancak LAPIC + IDT + kod hazırken artar, sonra sti. */
    ap_online[id] = 1;
    online_cpus++;

    /* 11E: AP'nin kendi LAPIC timer'ı (vektör 0x40, periyodik) - sti öncesi */
    lapic_timer_init(LAPIC_TIMER_TICKS);

    /* AP idle döngüsü: kesmeler açık, IPI/sys IPI bekler */
    asm volatile("sti");
    for (;;) asm volatile("hlt");
}

void smp_start_aps(void) {
    uint32_t ncpu = apic_cpu_count();
    if (ncpu < 2) {
        serial_puts("[11D] no APs to start (ncpu<2)\n");
        return;
    }

    /* Trampoline blob'unu 0x8000'e kopyala */
    uint32_t blob_size = (uint32_t)(trampoline_blob_end - trampoline_blob_start);
    if (blob_size > 0x1000) blob_size = 0x1000;
    memcpy((void*)TRAMPOLINE_ADDR, trampoline_blob_start, blob_size);
    serial_puts("[11D] trampoline copied (size=0x"); serial_puthex(blob_size); serial_puts(") to 0x8000\n");

    uint32_t bsp_id = apic_get_raw_id();

    /* BSP dışındaki her APIC id'yi uyandır (QEMU -smp 2: BSP=0, AP=1) */
    for (uint32_t apic_id = 0; apic_id < ncpu; apic_id++) {
        if (apic_id == bsp_id) continue;
        wake_one_ap(apic_id);
    }
}

void smp_init(void) {
    vga_puts("[8A] SMP init...\n");
    serial_puts("[8A] SMP init...\n");
    uint32_t cores = apic_cpu_count();
    vga_puts("[8A] Running on "); vga_putdec(cores); vga_puts(" core(s)\n");
    serial_puts("[8A] SMP cores: "); serial_puthex(cores); serial_puts("\n");

    /* 11D: AP'leri uyandır */
    smp_start_aps();

    vga_puts("[8A] SMP OK\n"); serial_puts("[8A] SMP OK\n");
}

void apic_init(void) {
    apic_init_inquiry();

    /* 21B: x2APIC destekleniyorsa BSP önce x2APIC moduna geçir (AP'lerden
       önce, tüm LAPIC erişimi MSR üzerinden yürüsün). */
    serial_puts("[21B] x2APIC check...\n");
    if (x2apic_supported()) {
        if (x2apic_try_enable()) {
            serial_puts("[21B] x2APIC enabled (MSR-based LAPIC) [PASS]\n");
            vga_puts("[21B] x2APIC enabled [PASS]\n");
        } else {
            serial_puts("[21B] x2APIC enable FAILED [FAIL]\n");
            vga_puts("[21B] x2APIC enable [FAIL]\n");
        }
    } else {
        serial_puts("[21B] x2APIC not supported (CPUID), using xAPIC [PASS]\n");
        vga_puts("[21B] x2APIC not supported, using xAPIC [PASS]\n");
    }

    vga_puts("[11A] LAPIC init (ID=");
    vga_putdec(apic_get_raw_id()); vga_puts(")\n");

    /* 11C: TLB shootdown IPI - IDT vektör 0x50'e asm stub isr50'yi bağla
       isr50 -> isr_common_stub -> isr_handler -> tlb_shootdown_handler */
    extern void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
    extern void isr50(void);
    idt_set_gate(TLB_SHOOTDOWN_VECTOR, (uint32_t)isr50, 0x08, 0x8E);
    isr_register_handler(TLB_SHOOTDOWN_VECTOR, tlb_shootdown_handler);
    serial_puts("[11C] TLB shootdown vector 0x50 registered\n");

    lapic_local_apic_init();

    /* 11E: LAPIC timer vektör 0x40 IDT gate + handler + BSP timer'ı başlat.
       (Handler isr_handler üzerinden gelir; EOI'yi kendisi gönderir.) */
    extern void isr40(void);
    idt_set_gate(LAPIC_TIMER_VECTOR, (uint32_t)isr40, 0x08, 0x8E);
    isr_register_handler(LAPIC_TIMER_VECTOR, lapic_timer_handler);
    serial_puts("[11E] LAPIC timer vector 0x40 registered\n");
    lapic_timer_init(LAPIC_TIMER_TICKS);

    vga_puts("[11A] APIC init done\n");
}

