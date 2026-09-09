/* 46J: hdacodec64/alsa64/mixer64/pulse64/jack64/btaudio64/src64/
 *      lowlat64/sndtest64/media64 host testi.
 * Calistirma: make test-audio64
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
    int loc = 0, dev = 0, conn = 0, color = 0;
    u8 nids[8];
    short pcm[256];
    u32 bf = 0, per = 0;
    u32 bw = 0;
    int op = 0;
    short r48[96], r16[32];
    short tone[480], back[480];
    int i;

    CHECK(hdacodec64_verb(0x12, 0xF1C, 0) ==
              ((u32)0x12 << 28 | (u32)0xF1C << 8), "46A komut");
    CHECK(hdacodec64_pin_config(0x00711000UL, &loc, &dev, &conn, &color) ==
              0 && loc == 0 && dev == 7 && conn == 1 && color == 1,
          "46A pin cozumu");
    CHECK(hdacodec64_widget_type(0x00400000UL) == 4, "46A widget turu");
    {
        u32 resps[1] = {0x000E0000UL | (3 << 8) | 9};
        CHECK(hdacodec64_fg_scan(resps, 1, nids, 8) == 3 && nids[0] == 9 &&
              nids[2] == 11, "46A grup tarama");
    }

    CHECK(alsa64_open(48000, 2, 16) == 0, "46B ac");
    CHECK(alsa64_open(0, 2, 16) != 0, "46B hiz red");
    CHECK(alsa64_open(48000, 2, 20) != 0, "46B bicim red");
    CHECK(alsa64_hw_params(48000, 2, &bf, &per) == 0 && bf == 4800 &&
          per == 1200, "46B hw params");
    CHECK(alsa64_writei(pcm, 10) != 0, "46B hazirliksiz red");
    CHECK(alsa64_prepare() == 0, "46B hazirla");
    memset(pcm, 0, sizeof(pcm));
    CHECK(alsa64_writei(pcm, 100) == 100, "46B yaz");
    CHECK(alsa64_state() == ALSA64_RUNNING, "46B calisiyor");
    CHECK(alsa64_avail() > 0, "46B bosluk");
    CHECK(alsa64_close() == 0, "46B kapat");

    CHECK(mixer64_add("Master", 0, 100) == 0, "46C ekle");
    CHECK(mixer64_set("Master", -1, 75) == 0, "46C tum kanallar");
    CHECK(mixer64_get("Master", 3) == 75, "46C oku");
    CHECK(mixer64_set("Master", 0, 999) == 0 &&
          mixer64_get("Master", 0) == 100, "46C kirpma");
    CHECK(mixer64_db_to_reg(0) == 192 && mixer64_reg_to_db(192) == 0,
          "46C dB donusum");
    CHECK(mixer64_get("Yok", 0) < -999999, "46C yok red");

    {
        int c = pulse64_client_add("muzik");
        int s1, s2;
        short mix[4];
        short a[2] = {1000, 1000};
        short b[2] = {2000, 2000};
        CHECK(c == 0, "46D istemci");
        s1 = pulse64_stream_open(c, 48000, 2);
        s2 = pulse64_stream_open(c, 48000, 2);
        CHECK(s1 >= 0 && s2 > s1, "46D akislar");
        CHECK(pulse64_write(s1, a, 1) == 1, "46D yaz1");
        CHECK(pulse64_write(s2, b, 1) == 1, "46D yaz2");
        CHECK(pulse64_mix(mix, 1) == 1 && mix[0] == 3000 &&
              mix[1] == 3000, "46D karisim");
        CHECK(pulse64_set_volume(s1, 50) == 0, "46D ses");
    }

    CHECK(jack64_set_rate(48000, 1024) == 0, "46E hiz");
    {
        int out = jack64_port_register("cikis", JACK64_AUDIO, 0);
        int in = jack64_port_register("giris", JACK64_AUDIO, 1);
        int midi = jack64_port_register("klavye", JACK64_MIDI, 0);
        CHECK(out >= 0 && in > out, "46E portlar");
        CHECK(jack64_connect(out, in) == 0, "46E bagla");
        CHECK(jack64_connect(in, out) != 0, "46E yon red");
        CHECK(jack64_connect(out, midi) != 0, "46E tur red");
        CHECK(jack64_cycle() == 1, "46E dongu");
    }

    CHECK(btaudio64_sbc_config(44100, 16, 8, 53, &bw) == 0 && bw > 0,
          "46D sbc");
    CHECK(btaudio64_sbc_config(44100, 7, 8, 53, 0) != 0,
          "46F blok red");
    CHECK(btaudio64_avrcp(0, &op) == 0 && op == 0x44, "46F oynat");
    CHECK(btaudio64_avrcp(9, 0) != 0, "46F komut red");
    {
        unsigned char mac[6] = {1, 2, 3, 4, 5, 6};
        CHECK(btaudio64_connect(mac) == 0, "46F baglan");
        CHECK(btaudio64_connect(mac) == 0, "46F yeniden");
    }

    CHECK(src64_open(48000, 16000) == 0, "46G ac");
    CHECK(src64_ratio() == 196608, "46G oran");
    for (i = 0; i < 96; i++) r48[i] = (short)(i * 100);
    {
        int n = src64_process(r48, 96, r16, 32);
        CHECK(n == 32, "46G 3:1 dusurme");
        CHECK(r16[0] == 0 && r16[1] == 300, "46G enterpolasyon");
    }
    CHECK(src64_process(r48, 1, r16, 32) != 0, "46G kisa red");
    CHECK(src64_open(0, 1) != 0, "46G hiz red");

    CHECK(lowlat64_period_frames(48000, 2000) == 128, "46H periyot");
    CHECK(lowlat64_latency_us(48000, 128) == 2666, "46H gecikme");
    CHECK(lowlat64_watermark(128) == 64, "46H esik");
    CHECK(lowlat64_policy(1) == 1, "46H gercek-zaman");

    CHECK(sndtest64_sine(440, 48000, 480, tone) == 480, "46I sinus");
    CHECK(tone[0] == 0, "46I sifir gecis");
    CHECK(!sndtest64_silence(tone, 480), "46I ses var");
    {
        short z[64];
        memset(z, 0, sizeof(z));
        CHECK(sndtest64_silence(z, 64), "46I sessizlik");
        z[10] = 32767;
        CHECK(sndtest64_clipping(z, 64), "46I kirpma");
        z[10] = 100;
        CHECK(!sndtest64_clipping(z, 64), "46I temiz");
    }
    CHECK(sndtest64_sweep(200, 2000, 48000, 100, back) == 100,
          "46I supurme");

    CHECK(media64_playlist_add("/muzik/a.ogg", 180000) == 0, "46J parca");
    CHECK(media64_playlist_add("/muzik/b.ogg", 240000) == 0, "46J parca2");
    CHECK(media64_play() == 0, "46J cal");
    CHECK(media64_state() == MEDIA64_PLAYING, "46J durum");
    CHECK(media64_position(60000) == 60000, "46J konum");
    CHECK(media64_pause() == 0, "46J duraklat");
    CHECK(media64_position(99999) == 0, "46J duraklatma dondurur");
    CHECK(media64_play() == 0 && media64_next() == 0, "46J sonraki");
    CHECK(media64_position(99999999UL) == 240000, "46J son kirpma");
    CHECK(media64_seek(1000) == 0, "46J ara");
    CHECK(media64_prev() == 0, "46J onceki");
    CHECK(media64_stop() == 0 && media64_state() == MEDIA64_STOPPED,
          "46J durdur");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
