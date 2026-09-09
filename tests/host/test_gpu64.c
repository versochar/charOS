/* 47J: gallium64/kms64/vk64/gles64/vram64/glcomp64/sched64/compute64/
 *      vaapi64 host testi.
 * Calistirma: make test-gpu64
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

int main(void) {
    u32 maxtex = 0;
    int low = -1;
    u32 w = 0, h = 0, r = 0;
    unsigned char guid[16];
    u64 bw = 0;

    CHECK(gallium64_init(512) == 0, "47A init");
    CHECK(gallium64_caps(&maxtex, &low) == 0 && maxtex == 2048 && low == 1,
          "47A dusuk profil");
    CHECK(gallium64_format_ok(1), "47A rgba8");
    CHECK(!gallium64_format_ok(99), "47A bilinmeyen red");
    CHECK(gallium64_init(0) != 0, "47A boyut red");

    {
        int c = kms64_add_connector("HDMI-1");
        CHECK(c >= 0, "47B baglayici");
        CHECK(kms64_add_mode(c, 1920, 1080, 60) == 0, "47B kip");
        CHECK(kms64_add_mode(c, 1280, 720, 60) == 0, "47B kip2");
        CHECK(kms64_set_mode(c, 1920, 1080) == 0, "47B kur");
        CHECK(kms64_current(c, &w, &h, &r) == 0 && w == 1920 && h == 1080 &&
              r == 60, "47B gecerli");
        CHECK(kms64_set_mode(c, 800, 600) != 0, "47B listesiz red");
    }

    CHECK(vk64_add_gpu("Iris Xe", 2048) == 0, "47C gpu");
    CHECK(vk64_add_gpu("llvmpipe", 512) == 1, "47C gpu2");
    CHECK(vk64_pick_gpu(512) == 1, "47C verimli secim");
    CHECK(vk64_pick_gpu(16384) < 0, "47C yetersiz red");
    CHECK(vk64_version_check(1, 0) == 0, "47C surum");
    CHECK(vk64_version_check(0, 9) != 0, "47C eski red");
    CHECK(vk64_queue_count(0) == 3, "47C kuyruk");

    CHECK(gles64_context(3, 2, 1024, 768) == 0, "47D baglam");
    CHECK(gles64_context(4, 0, 64, 64) != 0, "47D surum red");
    CHECK(gles64_viewport(0, 0, 800, 600) == 0, "47D gorunum");
    CHECK(gles64_viewport(900, 0, 800, 600) != 0, "47D tasma red");
    {
        int vs = gles64_shader(0, "#version 300 es\nvoid main(){gl_Position=vec4(0.);}");
        int fs = gles64_shader(1, "#version 300 es\nvoid main(){;}");
        CHECK(vs >= 0 && fs > vs, "47D derle");
        CHECK(gles64_link(vs, fs) == 0, "47D bagla");
        CHECK(gles64_shader(0, "bos") != 0, "47D kaynaksiz red");
        CHECK(gles64_link(fs, vs) != 0, "47D tur red");
    }

    CHECK(vram64_init(512ULL * 1024 * 1024) == 0, "47E init");
    {
        u64 a = vram64_alloc(1ULL * 1024 * 1024, 4096);
        u64 b = vram64_alloc(2ULL * 1024 * 1024, 2ULL * 1024 * 1024);
        CHECK(a != 0 && b != 0 && a != b, "47E tahsis");
        CHECK(b % (2ULL * 1024 * 1024) == 0, "47E hizalama");
        CHECK(vram64_alloc(1ULL * 1024 * 1024 * 1024, 4096) == 0,
              "47E asiri red");
        vram64_free(a);
        CHECK(vram64_mark_purgeable(b) == 0, "47E tasfiye isaret");
        CHECK(vram64_purge() == (int)(2ULL * 1024 * 1024),
              "47E tasfiye");
        CHECK(vram64_fragmentation() >= 0, "47E parcalanma");
    }

    {
        int s1 = glcomp64_surface(800, 600, 0xFF0000UL);
        int s2 = glcomp64_surface(100, 100, 0x00FF00UL);
        u32 h1, h2;
        CHECK(s1 >= 0 && s2 > s1, "47F yuzeyler");
        CHECK(glcomp64_damage(s2, 10, 10, 50, 50) == 0, "47F kirlet");
        CHECK(glcomp64_damage(s2, 900, 900, 10, 10) != 0,
              "47F dis red");
        h1 = glcomp64_present();
        h2 = glcomp64_present();
        CHECK(h1 != 0, "47F sunum karmasi");
        (void)h2;
        CHECK(glcomp64_vsync() == 2, "47F vsync");
    }

    {
        int hi = sched64_ctx_add(7, 1000);
        int lo = sched64_ctx_add(1, 1000);
        CHECK(hi >= 0 && lo > hi, "47G baglamlar");
        CHECK(sched64_submit(lo, 5) == 0, "47G gonder-dusuk");
        CHECK(sched64_submit(hi, 3) == 0, "47G gonder-yuksek");
        CHECK(sched64_run() == 8, "47G calistir");
        CHECK(sched64_run() == 0, "47G bos");
        CHECK(sched64_submit(lo, 4) == 0, "47G yeniden");
        CHECK(sched64_preempt(lo) == 0, "47G kes");
        CHECK(sched64_run() == 0, "47G kesik calismaz");
    }

    CHECK(compute64_groups(1000, 256) == 4, "47H grup hesabi");
    CHECK(compute64_groups(0, 256) == 0, "47H bos red");
    CHECK(compute64_dispatch(16, 16, 1, 256) == 0, "47H gonderim");
    CHECK(compute64_dispatch(0, 1, 1, 1) != 0, "47H boyut red");
    CHECK(compute64_shared_ok(16384), "47H paylasim ok");
    CHECK(!compute64_shared_ok(1ULL << 30), "47H asiri red");
    CHECK(compute64_completed() == 256, "47H tamamlanan");

    CHECK(vaapi64_profile_ok(0), "47I h264");
    CHECK(!vaapi64_profile_ok(9), "47I profil red");
    CHECK(vaapi64_config(0, 1920, 1080) == 0, "47I yapilandir");
    CHECK(vaapi64_surfaces(4, 1920, 1080) == 0, "47I yuzeyler");
    {
        unsigned char pkt[64];
        int i;
        for (i = 0; i < 64; i++) pkt[i] = (unsigned char)i;
        CHECK(vaapi64_decode(0, pkt, sizeof(pkt)) == 0, "47I coz");
        CHECK(vaapi64_decode(99, pkt, 1) != 0, "47I yuzey red");
        CHECK(vaapi64_frames() == 1, "47I kare sayac");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
