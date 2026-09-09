/* 55J: wayland64/xcompat64/inputv264/outputv264/virtkey64/screenshare64/
 *      remote64/secctx64/lowlatpath64 host testi.
 * Calistirma: make test-wserver64
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

static int method_hits = 0;

static int test_method(u32 obj, u16 opcode, u64 arg) {
    (void)obj;
    (void)opcode;
    method_hits += (int)arg + 1;
    return 0;
}

int main(void) {
    u32 id = 0, kc = 0;
    int pr = 0;
    u32 o = 0;
    u16 oc = 0;
    u64 oa = 0;
    int dev = -1, ty = 0, code = 0, val = 0;
    int ids[4];
    unsigned char frame[300];
    int rects[4];
    char prop[64];

    CHECK(wayland64_global_add("wl_compositor", 4) == 0, "55A global");
    CHECK(wayland64_bind("wl_compositor", 4, &id) == 0 && id, "55A bagla");
    CHECK(wayland64_bind("yok", 1, 0) != 0, "55A yok red");
    CHECK(wayland64_method(id, "wl_compositor", 1, test_method) == 0,
          "55A yontem");
    CHECK(wayland64_dispatch(id, 1, 5) == 0 && method_hits == 6,
          "55A dagitim");
    CHECK(wayland64_dispatch(id, 9, 0) != 0, "55A opcode red");
    CHECK(wayland64_dispatch(9999, 1, 0) != 0, "55A nesne red");
    CHECK(wayland64_event(id, 2, 7) == 0, "55A olay");
    CHECK(wayland64_poll(&o, &oc, &oa) == 0 && o == id && oc == 2 &&
          oa == 7, "55A yokla");
    CHECK(wayland64_poll(0, 0, 0) != 0, "55A bos red");

    CHECK(xcompat64_window_create(0x100, 3) == 0, "55B pencere");
    CHECK(xcompat64_prop_set(0x100, "WM_NAME", "Terminal") == 0,
          "55B ozellik");
    CHECK(xcompat64_prop_get(0x100, "WM_NAME", prop, sizeof(prop)) == 0 &&
          !strcmp(prop, "Terminal"), "55B oku");
    CHECK(xcompat64_prop_get(0x100, "YOK", prop, sizeof(prop)) != 0,
          "55B yok red");
    CHECK(xcompat64_map(0x100) == 0, "55B haritala");
    CHECK(xcompat64_mapped(0x100) == 1, "55B durum");
    CHECK(xcompat64_unmap(0x100) == 0, "55B kaldir");
    CHECK(xcompat64_mapped(0x100) == 0, "55B durum2");

    {
        int kb = inputv264_add(INPUTV264_KBD);
        int ms = inputv264_add(INPUTV264_PTR);
        CHECK(kb >= 0 && ms > kb, "55C cihazlar");
        CHECK(inputv264_add(0) != 0, "55C yeteneksiz red");
        CHECK(inputv264_emit(kb, 1, 30, 1, 1000) == 0, "55C gonder");
        CHECK(inputv264_poll(&dev, &ty, &code, &val) == 0 && dev == kb &&
              code == 30 && val == 1, "55C al");
        CHECK(inputv264_poll(0, 0, 0, 0) != 0, "55C bos red");
        CHECK(inputv264_grab(ms) == 0, "55C yakala");
        CHECK(inputv264_grab(99) != 0, "55C yok red");
    }

    {
        int hdmi = outputv264_add("HDMI-1");
        CHECK(hdmi >= 0, "55D cikti");
        CHECK(outputv264_mode(hdmi, 1920, 1080, 60) == 0, "55D kip");
        CHECK(outputv264_layout(hdmi, 0, 0, 1) == 0, "55D yerlesim");
        CHECK(outputv264_enable(hdmi, 1) == 0, "55D ac");
        CHECK(outputv264_list(ids, 4) == 1 && ids[0] == hdmi, "55D liste");
        CHECK(outputv264_enable(hdmi, 0) == 0, "55D kapat");
        CHECK(outputv264_list(ids, 4) == 0, "55D bos liste");
    }

    CHECK(virtkey64_press(0xE0) == 0, "55E degistirici");
    CHECK(virtkey64_mods() == 1, "55E mod durum");
    CHECK(virtkey64_press(0x04) == 0, "55E tus");
    CHECK(virtkey64_release(0xE0) == 0, "55E birak");
    CHECK(virtkey64_mods() == 0, "55E mod temiz");
    CHECK(virtkey64_read(&kc, &pr) == 0 && kc == 0xE0 && pr == 1,
          "55E oku");
    CHECK(virtkey64_read(&kc, &pr) == 0 && kc == 0x04, "55E oku2");

    {
        int st = screenshare64_start(0, 30);
        CHECK(st >= 0, "55F baslat");
        CHECK(screenshare64_frame(st, frame, sizeof(frame)) == 256,
              "55F kare");
        CHECK(screenshare64_subscribers(st) == 1, "55F abone");
        CHECK(screenshare64_frame(st, frame, 10) == 10, "55F kirpma");
        CHECK(screenshare64_stop(st) == 0, "55F durdur");
        CHECK(screenshare64_frame(st, frame, 10) != 0, "55F kapali red");
    }

    {
        /* "REMOTE12" LE: 52 45 4D 4F 54 45 31 32 */
        const char token[8] = {'R', 'E', 'M', 'O', 'T', 'E', '1', '2'};
        int se = remote64_hello("istemci", 1280, 720);
        CHECK(se >= 0, "55G merhaba");
        CHECK(remote64_input(se, 1, 2, 3) != 0, "55G kimliksiz red");
        CHECK(remote64_handshake(se, token) == 0, "55G el-sikisma");
        CHECK(remote64_input(se, 1, 2, 3) == 0, "55G girdi");
        CHECK(remote64_update(se, rects, 4) == 4 && rects[2] == 1280 &&
              rects[3] == 720, "55G guncelle");
        CHECK(remote64_handshake(se, "yanlis!") != 0, "55G jeton red");
        CHECK(remote64_disconnect(se) == 0, "55G kapat");
    }

    CHECK(secctx64_register("org.ornek", 1) == 0, "55H kayit");
    CHECK(secctx64_check("org.ornek", SECCTX64_SCREENCAST) == 0,
          "55H baslangic yoksun");
    CHECK(secctx64_prompt("org.ornek", SECCTX64_SCREENCAST) == 1,
          "55H portal bekliyor");
    CHECK(secctx64_grant("org.ornek", SECCTX64_SCREENCAST) == 0,
          "55H ver");
    CHECK(secctx64_check("org.ornek", SECCTX64_SCREENCAST) == 1,
          "55H denetle");
    CHECK(secctx64_revoke("org.ornek", SECCTX64_SCREENCAST) == 0,
          "55H geri al");
    CHECK(secctx64_check("org.ornek", SECCTX64_SCREENCAST) == 0,
          "55H kaldirildi");
    CHECK(secctx64_prompt("yok", 1) != 0, "55H kayitsiz red");

    {
        int sc = lowlatpath64_scanout(1920, 1080, 1);
        CHECK(sc >= 0, "55I tarama");
        CHECK(lowlatpath64_flip(sc) == 0, "55I cevir");
        CHECK(lowlatpath64_vblank(sc) == 16666, "55I vblank");
        CHECK(lowlatpath64_flip(sc) == 0, "55I cevir2");
        CHECK(lowlatpath64_vblank(sc) == 33332, "55I vblank2");
        CHECK(lowlatpath64_latency_us(sc) == 16666, "55I gecikme");
        CHECK(lowlatpath64_flip(99) != 0, "55I yok red");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
