/* 33.5: GERÇEK kernel/drivers/usbdesc.c testi (aynı dosya derlenir).
 * Düşman tanımlayıcılar dahil (BadUSB modeli).
 * Calistirma: make test-usbdesc33
 */
#include <stdio.h>
#include "drivers/usbdesc.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int8_t iface = -1;
    int proto = 0;
    uint8_t ep = 0, iv = 0;
    uint32_t mps = 0;

    /* 33.3: geçerli klavye */
    {
        static const uint8_t cfg[] = {
            9, 2, 25, 0, 1, 1, 0, 0x80, 50,
            9, 4, 0, 0, 1, 3, 1, 1, 0,
            7, 5, 0x81, 3, 8, 0, 10,
        };
        CHECK(usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &iface, &proto,
                                         &ep, &mps, &iv) == 0, "33.3 kbd");
        CHECK(iface == 0 && proto == 1 && ep == 0x81 && mps == 8 && iv == 10,
              "33.3 alanlar");
    }

    /* 33.3: geçerli fare (proto 2) */
    {
        static const uint8_t cfg[] = {
            9, 2, 25, 0, 1, 1, 0, 0x80, 50,
            9, 4, 1, 0, 1, 3, 1, 2, 0,
            7, 5, 0x82, 3, 4, 0, 5,
        };
        CHECK(usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &iface, &proto,
                                         &ep, &mps, &iv) == 0, "33.3 mouse");
        CHECK(iface == 1 && proto == 2 && ep == 0x82, "33.3 mouse-alan");
    }

    /* 33.3: L=0 düşman girdisi takılmadan reddedilir */
    {
        static const uint8_t evil[] = {9, 2, 9, 0, 0, 0, 0, 0, 0,
                                       0, 4, 0, 0, 0, 0, 0, 0, 0};
        CHECK(usbdesc_find_hid_interrupt(evil, sizeof(evil), &iface, &proto,
                                         &ep, &mps, &iv) == -1, "33.3 L0-red");
    }

    /* 33.3: kesik tanımlayıcı */
    {
        static const uint8_t cut[] = {9, 2, 25, 0, 1, 1, 0, 0x80, 50, 9, 4};
        CHECK(usbdesc_find_hid_interrupt(cut, sizeof(cut), &iface, &proto,
                                         &ep, &mps, &iv) == -1, "33.3 kesik");
    }

    /* 33.3: OUT uç (IN değil) reddedilir */
    {
        static const uint8_t cfg[] = {
            9, 2, 25, 0, 1, 1, 0, 0x80, 50,
            9, 4, 0, 0, 1, 3, 1, 1, 0,
            7, 5, 0x01, 3, 8, 0, 10,
        };
        CHECK(usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &iface, &proto,
                                         &ep, &mps, &iv) == -1, "33.3 out-red");
    }

    /* 33.3: HID-dışı sınıf reddedilir */
    {
        static const uint8_t cfg[] = {
            9, 2, 25, 0, 1, 1, 0, 0x80, 50,
            9, 4, 0, 0, 1, 8, 6, 80, 0,
            7, 5, 0x81, 2, 64, 0, 0,
        };
        CHECK(usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &iface, &proto,
                                         &ep, &mps, &iv) == -1, "33.3 sinif-red");
    }

    /* 33.3: parametre hataları */
    CHECK(usbdesc_find_hid_interrupt(0, 25, &iface, &proto, &ep, &mps, &iv) == -1,
          "33.3 null-cfg");
    {
        static const uint8_t cfg[9] = {0};
        CHECK(usbdesc_find_hid_interrupt(cfg, 4, &iface, &proto, &ep, &mps,
                                         &iv) == -1, "33.3 kisa-total");
        CHECK(usbdesc_find_hid_interrupt(cfg, 9, NULL, &proto, &ep, &mps,
                                         &iv) == -1, "33.3 null-out");
    }

    /* 33.3: öztest */
    CHECK(usbdesc_selftest() == 0, "33.3 selftest");

    /* 33.9: deterministik tekrar */
    {
        static const uint8_t cfg[] = {
            9, 2, 25, 0, 1, 1, 0, 0x80, 50,
            9, 4, 0, 0, 1, 3, 1, 1, 0,
            7, 5, 0x81, 3, 8, 0, 10,
        };
        int8_t i1 = -1, i2 = -1;
        int p1 = 0, p2 = 0;
        uint8_t e1 = 0, e2 = 0, v1 = 0, v2 = 0;
        uint32_t m1 = 0, m2 = 0;
        int r1 = usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &i1, &p1, &e1,
                                            &m1, &v1);
        int r2 = usbdesc_find_hid_interrupt(cfg, sizeof(cfg), &i2, &p2, &e2,
                                            &m2, &v2);
        CHECK(r1 == 0 && r2 == 0 && i1 == i2 && p1 == p2 && e1 == e2 &&
              m1 == m2 && v1 == v2, "33.9 deterministik");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
