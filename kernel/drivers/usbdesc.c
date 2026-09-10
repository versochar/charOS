/* 33.3: USB tanımlayıcı yürüyücü (usbhid.c içinden çıkarıldı, birebir mantık).
 * Güvenlik: L<2 veya pencere-dışı uzunlukta durur (L=0 takılmaz);
 * total üst sınırı çağıran tarafından budanır (enum 9..256 zorlar).
 * Aynı dosya çekirdekte ve host testinde derlenir.
 */
#include "drivers/usbdesc.h"
#include "core/verify.h"

/* 33.3: derleme-zamanı kanıtı */
STATIC_ASSERT(sizeof(uint8_t) == 1);

static uint16_t d_rd16(const uint8_t* p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

int usbdesc_find_hid_interrupt(const uint8_t* cfg, uint32_t total,
                               int8_t* out_iface, int* out_proto,
                               uint8_t* out_ep, uint32_t* out_mps,
                               uint8_t* out_interval) {
    uint32_t o = 0;
    int8_t cur_if = -1, cur_proto = 0;
    if (REQUIRE(cfg != 0, 0xE401) != 0) return -1;
    if (REQUIRE(total >= 9, 0xE402) != 0) return -1;
    if (REQUIRE(out_iface != 0 && out_proto != 0 && out_ep != 0 &&
                out_mps != 0 && out_interval != 0, 0xE403) != 0) return -1;
    while (o + 2 <= total) {
        uint8_t L = cfg[o], T = cfg[o + 1];
        if (L < 2 || o + L > total) break;
        if (T == 4 && L >= 9) { /* interface */
            cur_if = (int8_t)cfg[o + 2];
            cur_proto = 0;
            if (cfg[o + 5] == 3 && cfg[o + 6] == 1 &&
                (cfg[o + 7] == 1 || cfg[o + 7] == 2))
                cur_proto = (int8_t)cfg[o + 7];
        } else if (T == 5 && L >= 7) { /* endpoint */
            uint8_t ea = cfg[o + 2], at = cfg[o + 3];
            if (cur_proto && (at & 3u) == 3 && (ea & 0x80u)) {
                uint32_t mps = d_rd16(cfg + o + 4) & 0x7FFu;
                if (!mps) mps = 8;
                *out_iface = cur_if;
                *out_proto = cur_proto;
                *out_ep = ea;
                *out_mps = mps;
                *out_interval = cfg[o + 6];
                return 0;
            }
        }
        o += L;
    }
    return -1;
}

int usbdesc_selftest(void) {
    /* geçerli klavye tanımlayıcı: cfg9 + iface(9) + ep(7) */
    static const uint8_t good[] = {
        9, 2, 25, 0, 1, 1, 0, 0x80, 50,
        9, 4, 0, 0, 1, 3, 1, 1, 0,
        7, 5, 0x81, 3, 8, 0, 10,
    };
    int8_t iface = -1;
    int proto = 0;
    uint8_t ep = 0, iv = 0;
    uint32_t mps = 0;
    if (usbdesc_find_hid_interrupt(good, sizeof(good), &iface, &proto,
                                   &ep, &mps, &iv) != 0) return -1;
    if (iface != 0 || proto != 1 || ep != 0x81 || mps != 8 || iv != 10)
        return -2;
    /* L=0 düşman girdisi: takılmadan -1 */
    {
        static const uint8_t evil[] = {9, 2, 9, 0, 0, 0, 0, 0, 0,
                                       0, 4, 0, 0, 0, 0, 0, 0, 0};
        if (usbdesc_find_hid_interrupt(evil, sizeof(evil), &iface, &proto,
                                       &ep, &mps, &iv) != -1) return -3;
    }
    if (usbdesc_find_hid_interrupt(0, 25, &iface, &proto, &ep, &mps, &iv) != -1)
        return -4;
    return 0;
}
