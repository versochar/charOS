#ifndef CHAROS_DRIVERS_HDA_H
#define CHAROS_DRIVERS_HDA_H

#include <stdint.h>

/* 28E: Intel HD Audio (HDA) sürücüsü - Tiger Lake (11th Gen).
 * PCI class: 04:03:00 (Multimedia controller / Audio device).
 * Hedef: gerçek hoparlörden ses (temel codec init + register setup). */

/* HDA register ofsetleri (BAR0 üzerinden, 4KB MMIO) */
#define HDA_GCTL_OFFSET      0x08    /* Global Control */
#define HDA_WAKEEN_OFFSET    0x0C    /* Wake Enable */
#define HDA_STATESTS_OFFSET  0x0E    /* State Change Status */
#define HDA_GSTS_OFFSET      0x10    /* Global Status */
#define HDA_OUTPAY_OFFSET    0x18    /* Output Payload Capability */
#define HDA_INPAY_OFFSET     0x1C    /* Input Payload Capability */
#define HDA_INTCTL_OFFSET    0x20    /* Interrupt Control */
#define HDA_INTSTS_OFFSET    0x24    /* Interrupt Status */
#define HDA_CORBLBASE_OFFSET 0x40    /* CORB Lower Base Address */
#define HDA_CORBUBASE_OFFSET 0x44    /* CORB Upper Base Address */
#define HDA_CORBWP_OFFSET    0x48    /* CORB Write Pointer */
#define HDA_CORBRP_OFFSET    0x4A    /* CORB Read Pointer */
#define HDA_CORBCTL_OFFSET   0x4C    /* CORB Control */
#define HDA_RIRBLBASE_OFFSET 0x50    /* RIRB Lower Base Address */
#define HDA_RIRBUBASE_OFFSET 0x54    /* RIRB Upper Base Address */
#define HDA_RIRBWP_OFFSET    0x58    /* RIRB Write Pointer */
#define HDA_RINTCNT_OFFSET   0x5A    /* RIRB Interrupt Count */
#define HDA_RIRBCTL_OFFSET   0x5C    /* RIRB Control */
#define HDA_STREAM_DESC_OFFSET 0x80  /* Stream Descriptor (her stream) */

/* Codec komut yapısı (VERB) */
#define HDA_VERB_SET_AMP_GAIN    0x300
#define HDA_VERB_GET_AMP_GAIN    0xB00
#define HDA_VERB_SET_PIN_CTL     0x707
#define HDA_VERB_GET_PIN_CTL     0xF07
#define HDA_VERB_PIN_WIDGET_CTL  0x707

/* Codec widget: Pin Widget (0x21 gibi), Output Widget (0x11 gibi) */

/* HDA başlatma ve durum */
int hda_init(void);
int hda_present(void);

/* Temel codec komut gönder (polling, tek komut) */
int hda_send_verb(uint32_t nid, uint32_t verb, uint32_t param);

/* Self-test: controller tespit + basit codec probe */
int hda_selftest(void);

#endif
