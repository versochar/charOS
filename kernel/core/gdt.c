#include <core/gdt.h>
#include <stddef.h>

/* GDT: 7 giriş
 *   0 null, 1 kcode, 2 kdata, 3 ucode, 4 udata,
 *   5 BSP TSS (cpu 0), 6 AP TSS (cpu 1)
 * Tek paylaşılan GDT; her CPU'nun KENDİ TSS descriptor'ı vardır. Böylece
 * per-CPU GDT arraysi (AP'de ltr GPF veriyordu) yerine iki ayrı TSS slotu
 * kullanılır ve single-shared-esp0 race'i giderilir.
 */
#define GDT_TSS_BSP 5
#define GDT_TSS_AP  6
#define GDT_NENTRY  7
static struct gdt_entry gdt_entries[GDT_NENTRY];
static struct gdt_ptr gdt_ptr_reg;

/* Per-CPU TSS yapıları (BSS: 0x1190xx; tss_relocate ile kheap'e taşınabilir) */
static struct tss_entry tss[2];

static int _cpu(void) {
    extern int cpu_id_get(void);
    int c = cpu_id_get();
    return (c < 2) ? c : 0;
}

/* BSP (cpu0) TSS'e erişim (scheduler esp0 set için) */
static struct tss_entry* tss_cpu(int cpu) {
    if (cpu < 0 || cpu >= 2) cpu = 0;
    int slot = (cpu == 0) ? GDT_TSS_BSP : GDT_TSS_AP;
    uint32_t base = (uint32_t)gdt_entries[slot].base_low |
                    ((uint32_t)gdt_entries[slot].base_middle << 16) |
                    ((uint32_t)gdt_entries[slot].base_high << 24);
    if (base == 0) return &tss[cpu];
    return (struct tss_entry*)base;
}

/* Aktif CPU'nun TSS'i (tss_active) - çoğu erişim aktif CPU'dur */
static struct tss_entry* tss_active(void) { return tss_cpu(_cpu()); }

/* ============================================================
 * gdt_set_gate: GDT girişini doldur
 * ============================================================ */
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran)
{
    gdt_entries[num].base_low      = base & 0xFFFF;
    gdt_entries[num].base_middle   = (base >> 16) & 0xFF;
    gdt_entries[num].base_high     = (base >> 24) & 0xFF;

    gdt_entries[num].limit_low     = limit & 0xFFFF;
    gdt_entries[num].granularity   = (limit >> 16) & 0x0F;

    gdt_entries[num].granularity  |= gran & 0xF0;
    gdt_entries[num].access        = access;
}

/* Dahili: GDT'yi yükle ve segmentleri yenile */
static void gdt_flush_internal(void) {
    asm volatile(
        "lgdt %0\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        "ljmp $0x08, $1f\n"
        "1:\n"
        : : "m"(gdt_ptr_reg) : "eax", "memory"
    );
}

static void tss_flush_internal(uint16_t sel) {
    asm volatile("ltr %0" : : "r"(sel));
}

/* GDT flush wrapper (boot.s'deki eski stub yerine) */
void gdt_flush(void) {
    gdt_flush_internal();
}
void tss_flush(uint16_t sel) {
    tss_flush_internal(sel);
}

/* 11D: AP trampoline kernel GDT'sini yükleyebilsin diye sarmalayıcılar */
uint16_t gdt_get_limit(void) { return gdt_ptr_reg.limit; }
uint32_t gdt_get_base(void)   { return gdt_ptr_reg.base; }

/* ============================================================
 * gdt_init: paylaşılan GDT'yi iki TSS descriptor'ıyla kur ve yükle.
 * Hem BSP hem AP bu fonksiyonu çağırır (aynı GDT memory'sini yazarlar,
 * cr3 paylaşıldığından güvenlidir). Her CPU kendi TSS (5 veya 6) için LTR yapar.
 * ============================================================ */
void gdt_init(void)
{
    gdt_ptr_reg.limit = sizeof(struct gdt_entry) * GDT_NENTRY - 1;
    gdt_ptr_reg.base  = (uint32_t)&gdt_entries;

    /* Null descriptor */
    gdt_set_gate(0, 0, 0, 0, 0);

    /* Kernel Code: base=0, limit=4GB, access=0x9A (P=1, DPL=0, S=1, Type=0xA), gran=0xCF (4KB, 32-bit) */
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

    /* Kernel Data: base=0, limit=4GB, access=0x92 (P=1, DPL=0, S=1, Type=0x2), gran=0xCF */
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    /* User Code: base=0, limit=4GB, access=0xFA (P=1, DPL=3, S=1, Type=0xA), gran=0xCF */
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

    /* User Data: base=0, limit=4GB, access=0xF2 (P=1, DPL=3, S=1, Type=0x2), gran=0xCF */
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    int cpu = _cpu();
    int slot = (cpu == 0) ? GDT_TSS_BSP : GDT_TSS_AP;
    uint16_t sel = (cpu == 0) ? GDT_TSS : GDT_TSS_AP_SEL;

    /* H3 FIX: yalnızca KENDİ slotunu yaz. Paylaşılan GDT'de diğer CPU'nun
     * descriptor'ını ezmek (özellikle relocate sonrası kheap adresini BSS
     * ile değiştirmek) BSP/AP TR önbelleği ile GDT'yi tutarsız bırakır.
     * Diğer slot 0 kalırsa tss_cpu() zaten BSS'e düşer (base==0 kontrolü). */
    gdt_set_gate(slot, (uint32_t)&tss[cpu], sizeof(struct tss_entry) - 1, 0x89, 0x00);

    /* GDT'yi yükle */
    gdt_flush_internal();
    tss_flush_internal(sel);
}

/* ============================================================
 * tss_init: aktif CPU'nun TSS'ini temizle (gdt_init tarafından descriptor set edilir)
 * ============================================================ */
void tss_init(void)
{
    int cpu = _cpu();
    struct tss_entry* tp = &tss[cpu];
    for (size_t i = 0; i < sizeof(struct tss_entry); i++)
        ((uint8_t*)tp)[i] = 0;
    tp->ss0 = GDT_KERNEL_DATA;
    tp->esp0 = 0;
    tp->iomap_base = sizeof(struct tss_entry);
}

/* ============================================================
 * tss_set_stack: aktif CPU'nun kernel stack'ini (esp0) güncelle
 * ============================================================ */
void tss_set_stack(uint32_t ss0, uint32_t esp0)
{
    tss_active()->ss0 = ss0;
    tss_active()->esp0 = esp0;
}

/* 13F+SMP: belirli bir CPU'nun kernel stack'ini güncelle (scheduler her CPU için) */
void tss_set_stack_cpu(int cpu, uint32_t ss0, uint32_t esp0)
{
    tss_cpu(cpu)->ss0 = ss0;
    tss_cpu(cpu)->esp0 = esp0;
}

uint32_t tss_get_esp0(void) { return tss_active()->esp0; }
uint32_t tss_get_esp0_cpu(int cpu) { return tss_cpu(cpu)->esp0; }