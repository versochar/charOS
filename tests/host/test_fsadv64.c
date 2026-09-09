/* 50J: btrfs64/zfs64/sshfs64/nfsclient64/nfsserver64/smb64/luks64/
 *      overlay64/encfs64 host testi + kiyas.
 * Calistirma: make test-fsadv64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "arch/x86_64/longmode.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static unsigned char backend[8][1024];

static int be_rd(u64 fh, u64 off, void *buf, u64 len) {
    unsigned char *d;
    u64 i;
    if (fh >= 8 || !buf || off + len > 1024) return -1;
    d = (unsigned char *)buf;
    for (i = 0; i < len; i++) d[i] = backend[fh][off + i];
    return (int)len;
}

static int be_wr(u64 fh, u64 off, const void *buf, u64 len) {
    const unsigned char *s;
    u64 i;
    if (fh >= 8 || !buf || off + len > 1024) return -1;
    s = (const unsigned char *)buf;
    for (i = 0; i < len; i++) backend[fh][off + i] = s[i];
    return (int)len;
}

int main(void) {
    char list[4][48];
    char layer[64];
    char oname[32];
    unsigned char pkt[64], rb[64];
    u64 res = 0;
    int i;

    CHECK(btrfs64_subvol_create("kok", 5) == 0, "50A olustur");
    CHECK(btrfs64_subvol_create("veri", 256) == 0, "50A olustur2");
    CHECK(btrfs64_list(list, 4) == 2, "50A liste");
    CHECK(btrfs64_snapshot("veri", "veri-snap") == 0, "50A anlik");
    CHECK(btrfs64_subvol_delete("veri") != 0, "50A cocuklu red");
    CHECK(btrfs64_subvol_delete("veri-snap") == 0, "50A snap sil");
    CHECK(btrfs64_subvol_delete("veri") == 0, "50A sil");
    CHECK(btrfs64_subvol_delete("yok") != 0, "50A yok red");

    CHECK(zfs64_pool_create("tank", 1ULL << 40) == 0, "50B havuz");
    CHECK(zfs64_dataset_create("tank", "ev", "/ev") == 0, "50B veri");
    CHECK(zfs64_dataset_create("yok", "x", 0) != 0, "50B havuz red");
    CHECK(zfs64_scrub("tank") == 0, "50B yikama");
    CHECK(zfs64_health("tank") == 0, "50B saglikli");
    CHECK(zfs64_health("yok") != 0, "50B yok red");

    CHECK(sshfs64_connect("sunucu", "mitsune", 0) == 0, "50C baglan");
    CHECK(sshfs64_packet(1, 2, "ab", 2, pkt, sizeof(pkt)) != 0,
          "50C kimliksiz red");
    CHECK(sshfs64_auth("") != 0, "50C bos sifre red");
    CHECK(sshfs64_auth("gizli") == 0, "50C kimlik");
    CHECK(sshfs64_packet(1, 2, "ab", 2, pkt, sizeof(pkt)) == 11 &&
          pkt[0] == 0 && pkt[3] == 7 && pkt[4] == 1, "50C paket");
    {
        unsigned char attr[24] = {0, 0, 0, 0xF, 0, 0, 0, 0,
                                  0, 0, 0, 100, 0, 0, 0, 1,
                                  0, 0, 0, 2,  0, 0, 1, 0xA4};
        u64 sz = 0;
        u32 md = 0;
        CHECK(sshfs64_attr_parse(attr, sizeof(attr), &sz, &md) == 24 &&
              sz == 100 && md == 0x1A4, "50C oznitelik");
    }

    nfsclient64_set_backend(be_rd, be_wr);
    smb64_set_backend(be_rd, be_wr);
    memset(backend, 0, sizeof(backend));
    CHECK(nfsclient64_mount("sunucu", "/diski") == 0, "50D bagla");
    CHECK(nfsclient64_mount("sunucu", "goreceli") != 0, "50D yol red");
    CHECK(nfsclient64_rpc(100003, 3, 6, 0xAA, pkt, sizeof(pkt)) == 40,
          "50D rpc");
    CHECK(pkt[0] == 0 && pkt[3] == 0xAA && pkt[11] == 2 &&
          pkt[15] == 0xA3, "50D rpc alan");
    {
        const char *w = "nfs-verisi";
        char r[16];
        memset(r, 0, sizeof(r));
        CHECK(nfsclient64_write(1, 0, w, 10) == 10, "50D yaz");
        CHECK(nfsclient64_read(1, 0, r, 10) == 10 && !memcmp(r, w, 10),
              "50D oku");
    }

    CHECK(nfsserver64_export("/diski", "*") == 0, "50E disari aktar");
    CHECK(nfsserver64_export("goreceli", 0) != 0, "50E yol red");
    CHECK(nfsserver64_dispatch(0, 0, &res) == 0, "50E null");
    CHECK(nfsserver64_dispatch(3, 7, &res) == 0 && res == 0x1007,
          "50E lookup");
    CHECK(nfsserver64_dispatch(3, 0, 0) != 0, "50E bos red");
    CHECK(nfsserver64_dispatch(99, 0, 0) != 0, "50E bilinmeyen red");
    CHECK(nfsserver64_clients("/diski") == 1, "50E istemci");

    {
        u32 dials[3] = {0x0202, 0x0300, 0x0311};
        CHECK(smb64_negotiate(dials, 3) == 0, "50F pazarlik");
        CHECK(smb64_tree("PAYLASIM") == 1, "50F agac");
        CHECK(smb64_negotiate(dials, 0) != 0, "50F bos red");
        {
            const char *w = "smb-verisi";
            char r[16];
            memset(r, 0, sizeof(r));
            CHECK(smb64_write(2, 0, w, 10) == 10, "50F yaz");
            CHECK(smb64_read(2, 0, r, 10) == 10 && !memcmp(r, w, 10),
                  "50F oku");
        }
    }

    CHECK(luks64_format("aes-xts-plain64") == 0, "50G bicimle");
    CHECK(luks64_add_key(0, 0xDEAD) == 0, "50G anahtar");
    CHECK(luks64_add_key(0, 0) != 0, "50G sifir red");
    CHECK(luks64_unlock(0, 0xBEEF) != 0 && !luks64_is_open(),
          "50G yanlis red");
    CHECK(luks64_unlock(0, 0xDEAD) == 0 && luks64_is_open(),
          "50G ac");
    CHECK(luks64_lock() == 0 && !luks64_is_open(), "50G kilitle");

    CHECK(overlay64_mount("/ust", "/is") == 0, "50H bagla");
    CHECK(overlay64_add_lower("/alt1") == 0, "50H alt");
    CHECK(overlay64_add_lower("/alt2") == 0, "50H alt2");
    CHECK(overlay64_lookup("/a.txt", layer, sizeof(layer)) == 0 &&
          !strcmp(layer, "/alt1"), "50H birlesik");
    CHECK(overlay64_copy_up("/a.txt") == 0, "50H kopyala");
    CHECK(overlay64_lookup("/a.txt", layer, sizeof(layer)) == 0 &&
          !strcmp(layer, "/ust"), "50H ust kazanir");
    CHECK(overlay64_whiteout("/a.txt") == 0, "50H beyazlat");
    CHECK(overlay64_lookup("/a.txt", 0, 0) != 0, "50H gizli");

    {
        unsigned char key[32], plain[64], enc[64], dec[64];
        for (i = 0; i < 32; i++) key[i] = (unsigned char)(i + 1);
        for (i = 0; i < 64; i++) plain[i] = (unsigned char)(i * 3);
        CHECK(encfs64_set_key(key) == 0, "50I anahtar");
        CHECK(encfs64_set_key((const unsigned char[32]){0}) != 0,
              "50I sifir red");
        CHECK(encfs64_encrypt(7, plain, enc, 64) == 0, "50I sifrele");
        CHECK(memcmp(enc, plain, 64) != 0, "50I farkli");
        CHECK(encfs64_decrypt(7, enc, dec, 64) == 0 &&
              !memcmp(dec, plain, 64), "50I coz");
        CHECK(encfs64_name("belge.txt", oname, sizeof(oname)) == 0 &&
              strlen(oname) == 16, "50I ad karartma");
    }

    /* 50J kiyas: overlay lookup + encfs verimi */
    {
        clock_t t0 = clock();
        int k, n = 0;
        unsigned char tmp[64], o2[64];
        for (k = 0; k < 2000; k++) {
            if (overlay64_lookup("/b.txt", layer, sizeof(layer)) == 0)
                n++;
            encfs64_encrypt((u64)k, tmp, o2, 64);
            n++;
        }
        {
            double sec = (double)(clock() - t0) / CLOCKS_PER_SEC;
            double ops = sec > 0 ? (4000.0) / sec : 0;
            printf("50J kiyas: 4000 op %.3fsn (%.0f op/sn)\n", sec, ops);
            CHECK(n == 4000 && ops > 0, "50J benchmark");
        }
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
