/* 33A: SMP baslatma — INIT-SIPI-SIPI (x2APIC/MSR yok, klasik MMIO).
 * lapic parametresi gercek MMIO veya test tamponu olabilir; saf mantik.
 */
#include "arch/x86_64/longmode.h"

#define ICR_DELIVERY_INIT  (5U << 8)
#define ICR_DELIVERY_SIPI  (6U << 8)
#define ICR_LEVEL_ASSERT   (1U << 14)
#define ICR_TRIGGER_LEVEL  (1U << 15)
#define ICR_DEST_FIELD(id) ((u32)(id) << 24)

void smp64_delay(u64 loops) {
    while (loops--) { __asm__ volatile("pause"); }
}

static void icr_write(volatile u32 *lapic, u32 apic_id, u32 lo) {
    lapic[LAPIC_ICR_HI / 4] = ICR_DEST_FIELD(apic_id);
    lapic[LAPIC_ICR_LO / 4] = lo;
}

void smp64_send_init(volatile u32 *lapic, u32 apic_id) {
    if (!lapic) return;
    icr_write(lapic, apic_id,
              ICR_DELIVERY_INIT | ICR_TRIGGER_LEVEL | ICR_LEVEL_ASSERT);
    smp64_delay(10000);
    /* INIT deassert */
    icr_write(lapic, apic_id, ICR_DELIVERY_INIT | ICR_TRIGGER_LEVEL);
    smp64_delay(10000);
}

void smp64_send_sipi(volatile u32 *lapic, u32 apic_id, u8 vector) {
    if (!lapic) return;
    icr_write(lapic, apic_id, ICR_DELIVERY_SIPI | (u32)vector);
    smp64_delay(10000);
}

/* Klasik sira: INIT, 10ms, SIPI, 200us, SIPI. Donus: her zaman 0
 * (AP canliligi ayri yoklanir; gercek bekleme caller'da). */
int smp64_start_ap(volatile u32 *lapic, u32 apic_id, u8 vector) {
    if (!lapic) return -1;
    smp64_send_init(lapic, apic_id);
    smp64_send_sipi(lapic, apic_id, vector);
    smp64_send_sipi(lapic, apic_id, vector);
    return 0;
}
