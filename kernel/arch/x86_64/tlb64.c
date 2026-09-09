/* 33D: TLB shootdown IPI cercevesi — bekleyen maske + IPI gonderimi. */
#include "arch/x86_64/longmode.h"

static u64 tlb64_pending_mask = 0;
static int tlb64_ready = 0;

void tlb64_init(void) {
    tlb64_pending_mask = 0;
    tlb64_ready = 1;
}

int tlb64_request(int cpu) {
    if (!tlb64_ready || cpu < 0 || cpu >= MAX_CPUS64) return -1;
    tlb64_pending_mask |= (1ULL << cpu);
    return 0;
}

int tlb64_pending(int cpu) {
    if (cpu < 0 || cpu >= MAX_CPUS64) return 0;
    return (tlb64_pending_mask & (1ULL << cpu)) ? 1 : 0;
}

void tlb64_ack(int cpu) {
    if (cpu < 0 || cpu >= MAX_CPUS64) return;
    tlb64_pending_mask &= ~(1ULL << cpu);
    /* Gercek invalidation: invlpg/mov-cr3 caller'da (ISR baglami). */
}

int tlb64_send(volatile u32 *lapic, u32 apic_id) {
    if (!lapic) return -1;
    lapic[LAPIC_ICR_HI / 4] = apic_id << 24;
    /* Fixed delivery, vector TLB64_VECTOR */
    lapic[LAPIC_ICR_LO / 4] = (u32)TLB64_VECTOR;
    return 0;
}
