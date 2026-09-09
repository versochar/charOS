#ifndef CHAROS_DRIVERS_XHCI_H
#define CHAROS_DRIVERS_XHCI_H

#include <stdint.h>

/* 26B: xHCI USB3 host controller (Intel Tiger Lake 00:14.0 hedefi,
 * QEMU qemu-xhci ile doğrulanır). Polling modeli (MSI yok):
 * komut halkası + olay halkası timer/ana döngüden taranır. */

/* --- TRB (16B) --- */
struct xhci_trb {
    uint64_t param;
    uint32_t status;
    uint32_t control;
} __attribute__((packed));

/* TRB tipleri */
#define XHCI_TRB_NORMAL   1
#define XHCI_TRB_SETUP    2
#define XHCI_TRB_DATA     3
#define XHCI_TRB_STATUS   4
#define XHCI_TRB_LINK     6
#define XHCI_TRB_EN_SLOT  9
#define XHCI_TRB_ADDR_DEV 11
#define XHCI_TRB_CFG_EP   12
#define XHCI_TRB_NOOP_CMD 23
#define XHCI_TRB_EV_TX    32
#define XHCI_TRB_EV_CMD   33
#define XHCI_TRB_EV_PORT  34

/* TRB control bitleri */
#define TRB_C       (1u << 0)
#define TRB_TC      (1u << 1)
#define TRB_CHAIN   (1u << 4)
#define TRB_IOC     (1u << 5)
#define TRB_IDT     (1u << 6)   /* setup: veri TRB içinde (OVMF kanıtı!) */
#define TRB_DIR_IN  (1u << 16)
#define TRB_TX_IN   (3u << 16)  /* setup TRT=IN */
#define TRB_TX_OUT  (2u << 16)  /* setup TRT=OUT */
#define TRB_BSR     (1u << 9)   /* addr dev: block set-address */

/* Olay tamamlanma kodları */
#define EVCC_OK     1
#define EVCC_SHORT  13

/* --- kayıt offsetleri --- */
/* CAP (BAR+0) */
#define XHCI_CAPLEN   0x00
#define XHCI_HCIVER   0x02
#define XHCI_HCSP1    0x04
#define XHCI_HCSP2    0x08
#define XHCI_HCCP1    0x10
#define XHCI_DBOFF    0x14
#define XHCI_RTSOFF   0x18
/* OP (BAR+CAPLEN) */
#define XHCI_USBCMD   0x00
#define XHCI_USBSTS   0x04
#define XHCI_PAGESZ   0x08
#define XHCI_CRCR     0x18
#define XHCI_DCBAAP   0x30
#define XHCI_CONFIG   0x38
#define XHCI_PORTSC0  0x400
/* USBCMD/USBSTS */
#define USBCMD_RS     (1u << 0)
#define USBCMD_HCRST  (1u << 1)
#define USBSTS_HALT   (1u << 0)
#define USBSTS_CNR    (1u << 11)
/* PORTSC */
#define PORT_CCS  (1u << 0)
#define PORT_PED  (1u << 1)
#define PORT_PR   (1u << 4)
#define PORT_PP   (1u << 9)
#define PORT_CSC  (1u << 17)
#define PORT_PEC  (1u << 18)
#define PORT_WRC  (1u << 19)
#define PORT_OCC  (1u << 20)
#define PORT_PRC  (1u << 21)
#define PORT_PLC  (1u << 22)
#define PORT_CEC  (1u << 23)
#define PORT_CAS  (1u << 24)
#define PORT_RWC  (PORT_CSC|PORT_PEC|PORT_WRC|PORT_OCC|PORT_PRC|PORT_PLC|PORT_CEC|PORT_CAS)
/* Hız (PS field): 1=FS 2=LS 3=HS 4=SS */
#define XHCI_SPEED_FS 1
#define XHCI_SPEED_LS 2
#define XHCI_SPEED_HS 3
#define XHCI_SPEED_SS 4
/* Interrupter */
#define IMAN_IP  (1u << 0)
/* LEGSUP ext cap */
#define EXTCAP_LEGSUP 1
#define LEGSUP_BIOS_OWNED (1u << 16)
#define LEGSUP_OS_OWNED   (1u << 24)

/* EP tipleri */
#define EP_TYPE_CTRL    4
#define EP_TYPE_INT_IN  7

/* Input context add bayrakları: bit0=slot, bit1=EP0(DCI1) */
#define SLOT_CTX_ADD  0x3u
#define EP0_CTX_ADD   (1u << 1)

int xhci_init(void);      /* 0 = hazır, -1 = yok */
int xhci_present(void);
int xhci_selftest(void);  /* 0 PASS */
/* Olay halkasını tara (timer/ana döngü); usbhid içinden de çağrılır */
void xhci_poll(void);

/* --- komutlar (senkron; başarılıysa 0) --- */
int xhci_cmd_enable_slot(uint8_t* slot_id);
int xhci_cmd_address_device(uint8_t slot, uint32_t* in_ctx, int bsr);
int xhci_cmd_configure_ep(uint8_t slot, uint32_t* in_ctx);
uint8_t xhci_last_cc(void); /* son komut tamamlanma kodu (tanı) */

/* --- transferler (senkron kontrol / kayıtlı kesme) ---
 * slot: 1-based. dci: 1=EP0. */
int xhci_control(uint8_t slot, const uint8_t setup[8],
                 uint8_t* data, uint32_t len, int dir_in,
                 uint32_t* xfer_len);
/* Kesme IN EP kaydı: ring kur + input ctx'e yaz (configure öncesi).
 * dci: endpoint DCI (EP1-IN = 3). mps/interval: tanımlayıcıdan. */
int xhci_intr_ep_setup(uint8_t slot, uint8_t dci, uint16_t mps,
                       uint8_t interval, uint32_t* in_ctx_out_ep);
/* Kesme IN TD kuyrukla (normal TRB, IOC); tamamlanma xhci_poll'de işlenir.
 * cb: tamamlanınca çağrılır (status: bayt, residual hariç gerçek uzunluk). */
typedef void (*xhci_intr_cb)(uint8_t slot, uint8_t dci,
                             uint8_t* data, uint32_t len, uint8_t cc);
int xhci_intr_submit(uint8_t slot, uint8_t dci, uint8_t* buf, uint32_t len,
                     xhci_intr_cb cb);

/* usbhid'in kullandığı context kurulum yardımcıları */
void xhci_make_slot_ctx(uint32_t* ictx, uint32_t route, uint8_t speed,
                        uint8_t entries, uint8_t rhport);
void xhci_make_ep0_ctx(uint8_t slot, uint32_t* ictx, uint16_t mps);
/* Configure için input ctx'i mevcut output ctx'ten başlat (EDK2 yöntemi):
 * slot+EP0 kopyalanır, entries=dci_new, drop=0, add=(1<<dci_new). */
int xhci_cfg_begin(uint8_t slot, uint32_t* ictx, uint8_t dci_new);
uint8_t* xhci_ctrl_buf(void);
int xhci_port_count(void);
int xhci_port_reset_pub(int port);
/* Slot'un device context'ini DCBAA'ya işle (Address öncesi şart) */
void xhci_slot_commit(uint8_t slot);

#endif
