/* 45J: uas64/usbaudio64/uvc64/hubpower64/msi64/usbip64/hidparse64/
 *      virthid64/usbdevfs64 host testi.
 * Calistirma: make test-usb64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static u32 hid_bits = 0;
static u32 hid_n = 0;

static void hid_cb(u16 page, u16 usage, u32 bitpos, u32 bits, int is_in) {
    (void)page;
    (void)usage;
    (void)bitpos;
    if (is_in) {
        hid_bits += bits;
        hid_n++;
    }
}

/* Boot klavye rapor tanimlayici (standart 8B rapor). */
static const unsigned char kb_desc[] = {
    0x05, 0x01, /* Usage Page (Generic Desktop) */
    0x09, 0x06, /* Usage (Keyboard) */
    0xA1, 0x01, /* Collection (Application) */
    0x05, 0x07, /*   Usage Page (Key Codes) */
    0x19, 0xE0, /*   Usage Minimum (224) */
    0x29, 0xE7, /*   Usage Maximum (231) */
    0x15, 0x00, /*   Logical Minimum (0) */
    0x25, 0x01, /*   Logical Maximum (1) */
    0x75, 0x01, /*   Report Size (1) */
    0x95, 0x08, /*   Report Count (8) */
    0x81, 0x02, /*   Input (Data, Variable, Absolute) ;modifier */
    0x95, 0x01, /*   Report Count (1) */
    0x75, 0x08, /*   Report Size (8) */
    0x81, 0x01, /*   Input (Constant) ;reserved */
    0x95, 0x06, /*   Report Count (6) */
    0x75, 0x08, /*   Report Size (8) */
    0x15, 0x00, /*   Logical Minimum (0) */
    0x25, 0x65, /*   Logical Maximum (101) */
    0x19, 0x00, /*   Usage Minimum (0) */
    0x29, 0x65, /*   Usage Maximum (101) */
    0x81, 0x00, /*   Input (Data, Array) ;tuslar */
    0xC0        /* End Collection */
};

int main(void) {
    struct uas64_cmd cmd;
    u8 cdb[10] = {0x28, 0, 0, 0, 0, 1, 0, 0, 1, 0};
    u8 key = 0, asc = 0;
    int tag;
    u32 rates[8];
    unsigned char fdesc[14] = {11, 0x24, 0x02, 0, 0, 0, 0, 2,
                               0x80, 0xBB, 0x00, 0x44, 0xAC, 0x00};
    unsigned char guid[16];
    u64 bw = 0;
    int vec = 0;
    u64 maddr = 0;
    u32 mdata = 0, mctrl = 0;
    unsigned char msix[16] = {0, 0, 0xE0, 0xFE, 0, 0, 0, 0,
                              0x40, 0, 0, 0, 0, 0, 0, 0};
    unsigned char pkt[64], rb[8];
    u32 seq = 0;
    int status = -1;
    u32 kb = 0, nk = 0;
    u8 keys[6] = {4, 0, 0, 0, 0, 0};
    int bus = 0, addr = 0;
    unsigned char dd[18] = {18, 1, 0, 2, 0, 0, 0, 8,
                            0x34, 0x12, 0x78, 0x56, 0, 1, 2, 3,
                            4, 1};
    unsigned char rd[18];

    CHECK(uas64_build_cmd(0, cdb, sizeof(cdb), 5, &cmd) == 0 &&
          cmd.iu_id == 0x01 && cmd.tag == 5 && cmd.cdb[0] == 0x28,
          "45A komut IU");
    CHECK(uas64_build_cmd(9, cdb, 10, 0, &cmd) != 0, "45A lun red");
    {
        unsigned char sense[14] = {3, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 2, 0x3A};
        CHECK(uas64_parse_sense(sense, sizeof(sense), &key, &asc) == 0 &&
              key == 2 && asc == 0x3A, "45A sense");
        CHECK(uas64_parse_sense(sense, 5, 0, 0) != 0, "45A kisa red");
    }
    tag = uas64_tag_alloc();
    CHECK(tag == 0, "45A etiket");
    CHECK(uas64_abort(0) == 0, "45A iptal");
    uas64_tag_free(0);
    CHECK(uas64_abort(0) != 0, "45A bosta iptal red");

    CHECK(usbaudio64_parse_rates(fdesc, sizeof(fdesc), rates, 8) == 2 &&
          rates[0] == 48000 && rates[1] == 44100, "45B oranlar");
    CHECK(usbaudio64_parse_rates(fdesc, 5, rates, 8) != 0, "45B kisa red");
    CHECK(usbaudio64_rate_set(44100) == 0 &&
          usbaudio64_rate_get() == 44100, "45B hiz");
    CHECK(usbaudio64_rate_set(999999) != 0, "45B hiz red");
    CHECK(usbaudio64_volume_set(0, -6) == 0 &&
          usbaudio64_volume_get(0) == -6, "45B ses");
    CHECK(usbaudio64_volume_set(0, -200) != 0, "45B ses red");
    CHECK(usbaudio64_mute(0, 1) == 0, "45B sessiz");

    CHECK(uvc64_bandwidth(640, 480, 30, 0, &bw) == 0 &&
          bw == (u64)640 * 480 * 2 * 30, "45C bant");
    CHECK(uvc64_probe(640, 480, 30, 0, &bw) == 0, "45C probe kabul");
    CHECK(uvc64_probe(1920, 1080, 60, 0, 0) != 0, "45C USB2 asimi red");
    CHECK(uvc64_format_guid(0, guid) == 0 && guid[0] == 'Y' &&
          guid[1] == 'U', "45C guid");
    CHECK(uvc64_format_guid(9, 0) != 0, "45C bicim red");

    CHECK(hubpower64_set(0, HUBPOWER64_SUSPEND) == 0, "45D askıya al");
    CHECK(hubpower64_state(0) == HUBPOWER64_SUSPEND, "45D durum");
    CHECK(hubpower64_overcurrent(1) == 0, "45D asiri akim");
    CHECK(hubpower64_set(1, HUBPOWER64_ON) != 0, "45D kilitli red");
    {
        int ma = 0;
        CHECK(hubpower64_budget(480, &ma) == 0 && ma == 500,
              "45D butce HS");
        CHECK(hubpower64_budget(5000, &ma) == 0 && ma == 900,
              "45D butce SS");
    }

    {
        int is64 = 0, mmc = 0;
        CHECK(msi64_parse(0x008E, &is64, &mmc) == 0 && is64 == 1,
              "45E yetenek");
        CHECK(msi64_alloc_vector(&vec) == 0 && vec == 0x40, "45E vektor");
        CHECK(msi64_msg(&maddr, &mdata, vec, 2) == 0 &&
              maddr == 0xFEE02000ULL && mdata == 0x40, "45E ileti");
        CHECK(msi64_msg(0, 0, 0x10, 0) != 0, "45E vektor red");
        msi64_free_vector(vec);
        CHECK(msi64_alloc_vector(&vec) == 0 && vec == 0x40,
              "45E geri donusum");
        msi64_free_vector(vec);
        CHECK(msi64_msix_entry(msix, &maddr, &mdata, &mctrl) == 0 &&
              maddr == 0xFEE00000ULL && mdata == 0x40, "45E msix");
    }

    CHECK(usbip64_build_submit(7, 3, 2, 0, pkt, sizeof(pkt)) ==
              24 + 16,
          "45F gonderim");
    CHECK(pkt[0] == 0x01 && pkt[1] == 0x11 && pkt[3] == 1, "45F baslik");
    {
        unsigned char ret[24] = {0x01, 0x11, 0, 3, 0, 0, 0, 7,
                                 0,    0,    0, 0, 0, 0, 0, 0,
                                 0,    0,    0, 0, 0, 0, 0, 0};
        CHECK(usbip64_parse_ret(ret, sizeof(ret), &seq, &status) == 0 &&
              seq == 7 && status == 0, "45F yanit");
        CHECK(usbip64_parse_ret(ret, 5, 0, 0) != 0, "45F kisa red");
    }
    CHECK(usbip64_build_unlink(9, 7, pkt, sizeof(pkt)) == 24,
          "45F bag-coz");

    hid_bits = 0;
    hid_n = 0;
    CHECK(hidparse64_walk(kb_desc, sizeof(kb_desc), hid_cb) == 0,
          "45G yurume");
    CHECK(hid_n >= 2 && hid_bits >= 16, "45G girdi alani");
    CHECK(hidparse64_keyboard(kb_desc, sizeof(kb_desc), &kb, &nk) == 0 &&
          nk == 6 && kb == 48, "45G klavye ozeti (6x8)");
    CHECK(hidparse64_walk(kb_desc, 5, hid_cb) != 0, "45G kesik red");

    CHECK(virthid64_create(kb_desc, sizeof(kb_desc)) == 0, "45H olustur");
    CHECK(virthid64_inject_key(0x02, keys) == 0, "45H tus");
    CHECK(virthid64_inject_mouse(1, 10, -5) == 0, "45H fare");
    CHECK(virthid64_read(rb, sizeof(rb)) == 8 && rb[0] == 0x02 &&
          rb[2] == 4, "45H klavye raporu");
    CHECK(virthid64_read(rb, sizeof(rb)) == 8 && rb[0] == 1 &&
          rb[1] == 10, "45H fare raporu");
    CHECK(virthid64_read(rb, sizeof(rb)) == 0, "45H bos");

    CHECK(usbdevfs64_add(1, 2, 0x1234, 0x5678, 480) == 0, "45I ekle");
    CHECK(usbdevfs64_find(0x1234, 0x5678, &bus, &addr) == 0 && bus == 1 &&
          addr == 2, "45I bul");
    CHECK(usbdevfs64_find(0x9999, 0, 0, 0) != 0, "45I yok red");
    CHECK(usbdevfs64_desc(1, 2, dd, sizeof(dd)) == 0, "45I tanimlayici");
    CHECK(usbdevfs64_read_desc(1, 2, rd, sizeof(rd)) == 18 && rd[8] == 0x34,
          "45I oku");
    CHECK(usbdevfs64_remove(1, 2) == 0, "45I kaldir");
    CHECK(usbdevfs64_find(0x1234, 0x5678, 0, 0) != 0, "45I sil sonrasi");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
