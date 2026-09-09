#ifndef CHAROS_CORE_APIC_H
#define CHAROS_CORE_APIC_H

#include <stdint.h>

/* LAPIC Register Offsetleri (LAPIC Base = 0xFEE00000 varsayılan) */
#define LAPIC_ID        0x020   /* APIC ID Register (ID) */
#define LAPIC_VER       0x030   /* APIC version */
#define LAPIC_TPR       0x080   /* Task priority */
#define LAPIC_EOI       0x0B0   /* End of interrupt */
#define LAPIC_LDR       0x0D0   /* Logical destination register */
#define LAPIC_SVR       0x0F0   /* Spurious vector */
#define LAPIC_ICRL      0x300   /* ICR low bits (vector + delivery + shorthand) */
#define LAPIC_ICRH      0x310   /* ICR high bits (destination APIC ID) */
#define LAPIC_TMR0      0x320   /* Timer local vector table */
#define LAPIC_TMRICT    0x380   /* Timer initial count */
#define LAPIC_TMRDIV    0x3E0   /* Timer divide config */

/* ICR bitleri / IPI modları */
#define LAPIC_ICR_FIXED        0x00000000  /* delivery mode 000 */
#define LAPIC_ICR_INIT         0x00000500  /* delivery mode 101 (INIT) */
#define LAPIC_ICR_STARTUP      0x00000600  /* delivery mode 110 (SIPI) */
#define LAPIC_ICR_LEVEL        0x00008000  /* trigger mode: level */
#define LAPIC_ICR_ASSERT       0x00004000  /* level: assert */
#define LAPIC_ICR_DEST_SELF    0x00040000  /* shorthand: self */
#define LAPIC_ICR_DEST_ALL     0x00080000  /* shorthand: all including self */
#define LAPIC_ICR_DEST_OTHERS  0x000C0000  /* shorthand: all except self */

/* TLB shootdown IPI vektörü (IDT 0x50, 11C) */
#define TLB_SHOOTDOWN_VECTOR   0x50

/* 11E: LAPIC timer vektörü + modlar (IDT 0x40, PIC 32-47 ile çakışmaz) */
#define LAPIC_TIMER_VECTOR     0x40
#define LAPIC_TMR_MASK         0x00010000  /* bit 16: masked */
#define LAPIC_TMR_PERIODIC     0x00020000  /* bit 17: periyodik mod */
#define LAPIC_TMR_DIV1         0x0000000B  /* bölücü: /1 */

/* Maksimum desteklenen CPU */
#define MAX_CPU         4

/* 21B: x2APIC - IA32_APIC_BASE MSR (0x1B) bitleri */
#define MSR_APIC_BASE       0x1B
#define MSR_APIC_BASE_EN     (1u << 11)   /* EXTD: APIC enable (bit 11) */
#define MSR_APIC_BASE_X2     (1u << 10)   /* EN_X2APIC: x2APIC mode (bit 10) */
#define MSR_APIC_BASE_BSP    (1u << 8)    /* BSP flag (korunur) */

/* x2APIC LAPIC registerları MSR uzayında: 0x800 + (offset >> 4) */
#define X2APIC_MSR_BASE      0x800u
#define X2APIC_LAPIC_ID_MSR  0x802u       /* LAPIC ID */

/* 21B: x2APIC tespiti (CPUID leaf 1, ECX bit 21) */
int x2apic_supported(void);
/* 21B: mevcut CPU'yu x2APIC moduna geçirir (0 = başarısız/kapalı) */
int x2apic_try_enable(void);

/* Fonksiyonlar */
void apic_init(void);
void smp_init(void);
uint32_t cpuid_get_count(void);
uint32_t lapic_read(uint32_t reg);
void lapic_write(uint32_t reg, uint32_t val);
uint32_t apic_cpu_count(void);
uint32_t apic_get_raw_id(void);  /* 13F+SMP: fiziksel LAPIC ID (register/MSR) */
void lapic_eoi_send(void);
void tlb_shootdown(uint32_t virt);
void smp_start_aps(void); /* 11D: AP INIT-SIPI-SIPI ile uyandır */
void lapic_timer_init(uint32_t count); /* 11E: per-CPU LAPIC timer */
uint32_t lapic_tick_get(void); /* BSP LAPIC sayacı (tanılama ikinci saati) */

#endif
