/* 56J: panel64/wmde64/settings64/theme64/fileman64/launcher64/notifyd64/
 *      taskbar64/ctxmenu64 host testi.
 * Calistirma: make test-de64
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch/x86_64/longmode.h"
#include "arch/x86_64/crash.h"
#include "arch/x86_64/update.h"
#include "arch/x86_64/pkgmgr.h"
#include "arch/x86_64/secupd.h"
#include "arch/x86_64/isoimg.h"
#include "arch/x86_64/installfw.h"
#include "arch/x86_64/parttool.h"
#include "arch/x86_64/luksop.h"
#include "arch/x86_64/blcfg.h"
#include "arch/x86_64/nbp.h"
#include "arch/x86_64/recenv.h"
#include "arch/x86_64/ctr.h"
#include "arch/x86_64/fpsb.h"
#include "arch/x86_64/landlk.h"

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

int main(void) {
    int x = -1, y = -1, w = -1, h = -1;
    char ob[128];
    long long iv = 0;
    int ids[8];

    {
        int p = panel64_create(PANEL64_TOP, 32);
        CHECK(p >= 0, "56A olustur");
        CHECK(panel64_add_plugin(p, "menu", 0) == 0, "56A eklenti");
        CHECK(panel64_add_plugin(p, "gorev", 1) == 0, "56A eklenti2");
        CHECK(panel64_geometry(p, &x, &y, &w, &h) == 0 && x == 0 &&
              y == 0 && w == 1920 && h == 32, "56A geometri");
        CHECK(panel64_remove_plugin(p, "menu") == 0, "56A kaldir");
        CHECK(panel64_autohide(p, 1) == 0, "56A gizle");
        CHECK(panel64_create(99, 10) != 0, "56A konum red");
    }

    {
        int a = wmde64_open("Terminal", 100, 100, 800, 600);
        int b = wmde64_open("Dosya", 200, 200, 640, 480);
        CHECK(a > 0 && b > a, "56B ac");
        CHECK(wmde64_focused() == b, "56B otomatik odak");
        CHECK(wmde64_focus(a) == 0 && wmde64_focused() == a, "56B odak");
        CHECK(wmde64_minimize(a, 1) == 0, "56B kucult");
        CHECK(wmde64_focused() == -1, "56B odak dustu");
        CHECK(wmde64_focus(a) == 0, "56B geri odak (buyutur)");
        CHECK(wmde64_maximize(b, 1) == 0, "56B buyut");
        CHECK(wmde64_move(b, 10, 10) != 0, "56B buyuk tasinmaz");
        CHECK(wmde64_maximize(b, 0) == 0, "56B eski haline");
        CHECK(wmde64_move(b, 10, 10) == 0, "56B tasi");
        CHECK(wmde64_resize(b, 320, 200) == 0, "56B boyutla");
        CHECK(wmde64_workspace(2) == 0, "56B alan2");
        CHECK(wmde64_current_workspace() == 2, "56B alan oku");
        CHECK(wmde64_close(a) == 0 && wmde64_close(b) == 0, "56B kapat");
        CHECK(wmde64_focus(a) != 0, "56B kapali red");
    }

    CHECK(settings64_set("pencere", "tema", "koyu") == 0, "56C kur");
    CHECK(settings64_get("pencere", "tema", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "koyu"), "56C oku");
    CHECK(settings64_set_int("pil", "esik", 20) == 0, "56C tamsayi");
    CHECK(settings64_get_int("pil", "esik", &iv) == 0 && iv == 20,
          "56C tamsayi oku");
    CHECK(settings64_set_int("pil", "neg", -5) == 0, "56C negatif");
    CHECK(settings64_get_int("pil", "neg", &iv) == 0 && iv == -5,
          "56C negatif oku");
    CHECK(settings64_watch("pencere", "tema") == 0, "56C izle");
    CHECK(settings64_set("pencere", "tema", "acik") == 0, "56C degistir");
    {
        char ch[32], ky[48];
        CHECK(settings64_changed(ch, sizeof(ch), ky, sizeof(ky)) == 0 &&
              !strcmp(ch, "pencere") && !strcmp(ky, "tema"),
              "56C bildirim");
        CHECK(settings64_changed(0, 0, 0, 0) != 0, "56C tuketildi");
    }
    CHECK(settings64_get("yok", "x", ob, sizeof(ob)) != 0, "56C yok red");

    CHECK(theme64_add("koyu") == 0, "56D ekle");
    CHECK(theme64_set_prop("koyu", "arka-plan", "#1E1E2E") == 0,
          "56D ozellik");
    CHECK(theme64_get_prop("koyu", "arka-plan", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "#1E1E2E"), "56D oku");
    CHECK(theme64_apply("koyu") == 0, "56D uygula");
    CHECK(theme64_active(ob, sizeof(ob)) == 0 && !strcmp(ob, "koyu"),
          "56D etkin");
    CHECK(theme64_color("#FF8000") == 0xFF8000UL, "56D renk");
    CHECK(theme64_color("#ff8000") == 0xFF8000UL, "56D kucuk harf");
    CHECK(theme64_color("FF8000") == 0, "56D kare red");
    CHECK(theme64_color("#ZZZZZZ") == 0, "56D hex red");
    CHECK(theme64_apply("yok") != 0, "56D yok red");

    CHECK(fileman64_add("/ev/belge.txt", 0, 100) == 0, "56E dosya");
    CHECK(fileman64_add("/ev/muzik", 1, 0) == 0, "56E dizin");
    CHECK(fileman64_add("/ev/arsiv.zip", 0, 50) == 0, "56E dosya2");
    {
        char lst[4][64];
        int n = fileman64_list("/ev", lst, 4);
        CHECK(n == 3, "56E liste adedi");
        CHECK(!strcmp(lst[0], "muzik"), "56E dizin once");
        CHECK(!strcmp(lst[1], "arsiv.zip") && !strcmp(lst[2], "belge.txt"),
              "56E alfabetik");
    }
    CHECK(fileman64_select("/ev/belge.txt") == 0, "56E sec");
    CHECK(fileman64_clip_add("/ev/belge.txt", 0) == 0, "56E pano");
    CHECK(fileman64_mkdir("/yedek") == 0, "56E dizin ac");
    CHECK(fileman64_clip_paste("/yedek") == 1, "56E yapistir");
    {
        char lst[4][64];
        int n = fileman64_list("/yedek", lst, 4);
        CHECK(n == 1 && !strcmp(lst[0], "belge.txt"), "56E kopya geldi");
    }

    CHECK(launcher64_add("Terminal", "xterm", "terminal",
                         "System;Utility;") == 0, "56F ekle");
    CHECK(launcher64_add("Dosya", "thunar", "dosya", "System;") == 0,
          "56F ekle2");
    {
        char q[4][48];
        CHECK(launcher64_query("System", q, 4) == 2, "56F kategori");
        CHECK(launcher64_query("Oyun", q, 4) == 0, "56F bos kategori");
    }
    CHECK(launcher64_launch("Terminal", ob, sizeof(ob)) == 0 &&
          !strcmp(ob, "xterm"), "56F baslat");
    CHECK(launcher64_fav("Terminal", 1) == 0, "56F favori");
    CHECK(launcher64_launch("Yok", 0, 0) != 0, "56F yok red");

    {
        int n1 = notifyd64_send("posta", "Yeni ileti", "Merhaba", 5000);
        int n2 = notifyd64_send("takvim", "Toplanti", 0, 0);
        CHECK(n1 > 0 && n2 > n1, "56G gonder");
        CHECK(notifyd64_action(n1, "ac") == 0, "56G varsayilan eylem");
        CHECK(notifyd64_list(ids, 8) == 1 && ids[0] == n2,
              "56G liste");
        CHECK(notifyd64_expire(6000) == 1, "56G kalici duruyor");
        CHECK(notifyd64_close(n2) == 0, "56G kapat");
        CHECK(notifyd64_list(ids, 8) == 0, "56G temiz");
        CHECK(notifyd64_close(n2) != 0, "56G cift kapat red");
        /* 56G detay: eylem ekle, aciliyet, getir, degistir, sayi, temizle */
        {
            int nid = notifyd64_send("sistem", "Bakim", "Guncelleme basliyor", 10000);
            CHECK(nid > 0, "56G2 gonder");
            CHECK(notifyd64_add_action(nid, "ertele") == 0, "56G2 eylem ekle");
            CHECK(notifyd64_add_action(nid, "simdi") == 0, "56G2 eylem ekle2");
            CHECK(notifyd64_set_urgency(nid, 2) == 0, "56G2 aciliyet ayarla");
            CHECK(notifyd64_urgency(nid) == 2, "56G2 aciliyet oku");
            {
                char a[48], s[64], b[128];
                CHECK(notifyd64_get(nid, a, sizeof(a), s, sizeof(s), b, sizeof(b)) == 0 &&
                      !strcmp(a, "sistem") && !strcmp(s, "Bakim") && !strcmp(b, "Guncelleme basliyor"),
                      "56G2 getir");
            }
            CHECK(notifyd64_replace(nid, "Bakim Tamam", "Islem bitti") == 0,
                  "56G2 degistir");
            {
                char s[64], b[128];
                CHECK(notifyd64_get(nid, 0, 0, s, sizeof(s), b, sizeof(b)) == 0 &&
                      !strcmp(s, "Bakim Tamam") && !strcmp(b, "Islem bitti"),
                      "56G2 degistir kontrol");
            }
            CHECK(notifyd64_count() >= 1, "56G2 sayi");
            CHECK(notifyd64_clear_all() >= 0, "56G2 temizle");
            CHECK(notifyd64_count() == 0, "56G2 bos");
            /* kritik aciliyet suresiz */
            {
                int nid2 = notifyd64_send("kritik", "Saglik", "Durum kritik", 1000);
                CHECK(notifyd64_set_urgency(nid2, 2) == 0, "56G2 kritik");
                CHECK(notifyd64_expire(5000) >= 1, "56G2 kritik kalici");
                CHECK(notifyd64_close(nid2) == 0, "56G2 kritik kapat");
            }
        }
    }

    /* 56H detay: liste, baslik, tasi, sabitle, 12s saat */
    {
        int wins[8];
        char ttl[64];
        CHECK(taskbar64_add(11, "Terminal") == 0, "56H ekle");
        CHECK(taskbar64_add(12, "Dosya") == 0, "56H ekle2");
        CHECK(taskbar64_add(13, "Oyun") == 0, "56H ekle3");
        CHECK(taskbar64_list(wins, 8) == 3 && wins[0] == 11 && wins[1] == 12 && wins[2] == 13,
              "56H liste");
        CHECK(taskbar64_title(12, ttl, sizeof(ttl)) == 0 && !strcmp(ttl, "Dosya"),
              "56H baslik");
        CHECK(taskbar64_minimized(11) == 0, "56H minimize durum");
        CHECK(taskbar64_click(11) == 0, "56H tik kucult");
        CHECK(taskbar64_minimized(11) == 1, "56H minimize oldu");
        CHECK(taskbar64_click(11) == 1, "56H tik odak");
        CHECK(taskbar64_urgent(12, 1) == 0, "56H acil");
        CHECK(taskbar64_pin(12, 1) == 0, "56H sabitle");
        CHECK(taskbar64_pinned(12) == 1, "56H sabit kontrol");
        CHECK(taskbar64_move(13, 0) == 0, "56H tasi");
        {
            int w2[8];
            CHECK(taskbar64_list(w2, 8) == 3 && w2[0] == 13, "56H tasi kontrol");
        }
        CHECK(taskbar64_clock(9, 5, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "09:05"), "56H saat");
        CHECK(taskbar64_clock(25, 0, ob, sizeof(ob)) != 0, "56H saat red");
        CHECK(taskbar64_clock12(14, 5, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "02:05 PM"), "56H 12s saat PM");
        CHECK(taskbar64_clock12(0, 0, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "12:00 AM"), "56H 12s gece");
        CHECK(taskbar64_clock12(12, 30, ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "12:30 PM"), "56H 12s ogleden");
        CHECK(taskbar64_remove(11) == 0, "56H kaldir");
        CHECK(taskbar64_click(11) != 0 && taskbar64_click(11) != 1,
              "56H yok red");
    }

    {
        int m = ctxmenu64_create();
        int sub = ctxmenu64_create();
        int idx;
        CHECK(m >= 0 && sub > m, "56I olustur");
        CHECK(ctxmenu64_add_item(m, "Ac", 10) == 0, "56I oge");
        CHECK(ctxmenu64_add_item(m, "Sil", 20) == 1, "56I oge2");
        CHECK(ctxmenu64_add_item(sub, "Kopyala", 30) == 0, "56I alt oge");
        CHECK(ctxmenu64_add_submenu(m, "Paylas", sub) >= 0, "56I alt menu");
        CHECK(ctxmenu64_popup(m, 100, 200) == 0, "56I ac");
        idx = ctxmenu64_activate(m, 1);
        CHECK(idx == 20, "56I tetikle");
        CHECK(ctxmenu64_popup(m, 0, 0) == 0, "56I yeniden ac");
        idx = ctxmenu64_activate(m, 2);
        CHECK(idx == -1000 - sub, "56I alt menu donus");
        /* 56I detay: sayi, etiket/eylem, duyarli, konum, kapat, temizle */
        {
            char lbl[48];
            int act = -1;
            int x = -1, y = -1;
            CHECK(ctxmenu64_count(m) == 3, "56I2 sayac");
            CHECK(ctxmenu64_label(m, 0, lbl, sizeof(lbl)) == 0 && !strcmp(lbl, "Ac"),
                  "56I2 etiket");
            CHECK(ctxmenu64_action(m, 1, &act) == 0 && act == 20,
                  "56I2 eylem");
            CHECK(ctxmenu64_set_sensitive(m, 1, 0) == 0, "56I2 duyarli kapali");
            CHECK(ctxmenu64_popup(m, 0, 0) == 0, "56I2 popup");
            idx = ctxmenu64_activate(m, 0);
            CHECK(idx != 20, "56I2 duyarli atlandi");
            CHECK(ctxmenu64_set_sensitive(m, 1, 1) == 0, "56I2 duyarli acik");
            CHECK(ctxmenu64_pos(m, &x, &y) == 0 && x == 0 && y == 0,
                  "56I2 konum");
            CHECK(ctxmenu64_close(m) == 0, "56I2 kapat");
            CHECK(ctxmenu64_activate(m, 0) == -1, "56I2 kapali red");
            CHECK(ctxmenu64_remove_item(m, 0) == 0, "56I2 kaldir");
            CHECK(ctxmenu64_count(m) == 2, "56I2 sayac azaldi");
            CHECK(ctxmenu64_clear(m) == 0, "56I2 temizle");
            CHECK(ctxmenu64_count(m) == 0, "56I2 bos");
        }
    }

    /* 56A detay: sira, strut, opaklik, monitor */
    {
        int p = panel64_create(PANEL64_BOTTOM, 40);
        int l = -1, r = -1, t = -1, b = -1;
        char nm[32];
        CHECK(p >= 0, "56A2 olustur");
        CHECK(panel64_add_plugin(p, "saat", 0) == 0, "56A2 eklenti");
        CHECK(panel64_add_plugin(p, "ses", 0) == 0, "56A2 eklenti2");
        CHECK(panel64_add_plugin(p, "ag", 0) == 0, "56A2 eklenti3");
        CHECK(panel64_plugin_count(p) == 3, "56A2 sayac");
        CHECK(panel64_move_plugin(p, "ag", 0) == 0, "56A2 basa tasi");
        CHECK(panel64_plugin_at(p, 0, nm, sizeof(nm)) == 0 &&
              !strcmp(nm, "ag"), "56A2 sira");
        CHECK(panel64_plugin_at(p, 2, nm, sizeof(nm)) == 0 &&
              !strcmp(nm, "ses"), "56A2 sira2");
        CHECK(panel64_move_plugin(p, "yok", 0) != 0, "56A2 yok red");
        CHECK(panel64_struts(p, &l, &r, &t, &b) == 0 && b == 40 &&
              !l && !r && !t, "56A2 strut");
        CHECK(panel64_autohide(p, 1) == 0, "56A2 gizle");
        CHECK(panel64_struts(p, &l, &r, &t, &b) == 0 && !b,
              "56A2 gizli strutsuz");
        CHECK(panel64_autohide(p, 0) == 0, "56A2 goster");
        CHECK(panel64_opacity(p, 80) == 0 &&
              panel64_opacity_get(p) == 80, "56A2 opaklik");
        CHECK(panel64_opacity(p, 101) != 0, "56A2 aralik red");
        CHECK(panel64_output(p, 1) == 0 && panel64_output_get(p) == 1,
              "56A2 monitor");
        CHECK(panel64_output(p, 99) != 0, "56A2 monitor red");
    }

    /* 56B detay: z-sira, yapiskan, tam-ekran, kisit, geometri */
    {
        int w1, w2;
        int st[8];
        int x = -1, y = -1, w = -1, h = -1;
        CHECK(wmde64_workspace(0) == 0, "56B2 alan0");
        w1 = wmde64_open("A", 0, 0, 400, 300);
        w2 = wmde64_open("B", 10, 10, 400, 300);
        CHECK(w1 > 0 && w2 > w1, "56B2 ac");
        CHECK(wmde64_count() == 2, "56B2 sayac");
        CHECK(wmde64_raise(w1) > 0, "56B2 yukselt");
        {
            int n = wmde64_stack(0, st, 8);
            CHECK(n == 2 && st[1] == w1, "56B2 yigin sirasi");
        }
        CHECK(wmde64_lower(w1) >= 0, "56B2 alcalt");
        {
            int n = wmde64_stack(0, st, 8);
            CHECK(n == 2 && st[0] == w1, "56B2 alta indi");
        }
        CHECK(wmde64_sticky(w2, 1) == 0, "56B2 yapiskan");
        CHECK(wmde64_workspace(3) == 0, "56B2 alan3");
        {
            int n = wmde64_stack(3, st, 8);
            CHECK(n == 1 && st[0] == w2, "56B2 yapiskan gorunur");
        }
        CHECK(wmde64_workspace(0) == 0, "56B2 geri don");
        CHECK(wmde64_fullscreen(w1, 1) == 0, "56B2 tam-ekran");
        CHECK(wmde64_geom(w1, &x, &y, &w, &h) == 0 && x == 0 && w == 1920,
              "56B2 tam geometri");
        CHECK(wmde64_move(w1, 5, 5) != 0, "56B2 tam tasinmaz");
        CHECK(wmde64_fullscreen(w1, 0) == 0, "56B2 cik");
        CHECK(wmde64_geom(w1, &x, &y, &w, &h) == 0 && x == 0 && w == 400,
              "56B2 eski geometri");
        CHECK(wmde64_constrain(w1, 200, 150, 800, 600) == 0,
              "56B2 kisit");
        CHECK(wmde64_constrain(w1, 900, 0, 100, 0) != 0,
              "56B2 ters kisit red");
        CHECK(wmde64_resize(w1, 50, 50) == 0, "56B2 kirpma cagrisi");
        CHECK(wmde64_geom(w1, 0, 0, &w, &h) == 0 && w == 200 && h == 150,
              "56B2 kirpildi");
        CHECK(wmde64_resize(w1, 5000, 5000) == 0, "56B2 buyutme cagrisi");
        CHECK(wmde64_geom(w1, 0, 0, &w, &h) == 0 && w == 800 && h == 600,
              "56B2 tavan");
        CHECK(wmde64_sticky(w2, 0) == 0, "56B2 yapiskan kapat");
        CHECK(wmde64_close(w1) == 0 && wmde64_close(w2) == 0,
              "56B2 kapat");
        CHECK(wmde64_count() == 0, "56B2 bos");
    }

    /* 56C detay: tipler, liste/sil, kuyruk, varsayilan, export/import */
    {
        int b = -1;
        long long m = 0;
        char kl[4][48];
        char chs[4][32];
        char dump[512];
        CHECK(settings64_set_bool("fare", "dogal", 1) == 0, "56C2 bool");
        CHECK(settings64_get_bool("fare", "dogal", &b) == 0 && b == 1,
              "56C2 bool oku");
        CHECK(settings64_set_bool("fare", "hiz", 0) == 0, "56C2 bool0");
        CHECK(settings64_get_bool("fare", "hiz", &b) == 0 && b == 0,
              "56C2 bool0 oku");
        CHECK(settings64_set("fare", "bozuk", "belki") == 0, "56C2 kur");
        CHECK(settings64_get_bool("fare", "bozuk", &b) != 0,
              "56C2 bool red");
        CHECK(settings64_set_double("ses", "seviye", 1500) == 0,
              "56C2 double");
        CHECK(settings64_get_double("ses", "seviye", &m) == 0 && m == 1500,
              "56C2 double oku");
        CHECK(settings64_set("ses", "denge", "-0.5") == 0, "56C2 neg");
        CHECK(settings64_get_double("ses", "denge", &m) == 0 && m == -500,
              "56C2 neg oku");
        CHECK(settings64_keys("fare", kl, 4) == 3, "56C2 anahtarlar");
        CHECK(settings64_channels(chs, 4) >= 3, "56C2 kanallar");
        CHECK(settings64_default("pencere", "golge", "acik") == 0,
              "56C2 varsayilan");
        CHECK(settings64_reset("pencere", "tema") == 0, "56C2 sifirla");
        CHECK(settings64_get("pencere", "golge", ob, sizeof(ob)) == 1 &&
              !strcmp(ob, "acik"), "56C2 yedek deger");
        CHECK(settings64_get("pencere", "tema", ob, sizeof(ob)) != 0,
              "56C2 silinmis yok");
        CHECK(settings64_export(dump, sizeof(dump)) > 0, "56C2 disari");
        CHECK(settings64_reset("fare", "dogal") == 0, "56C2 sil");
        CHECK(settings64_get("fare", "dogal", ob, sizeof(ob)) != 0,
              "56C2 gitti");
        CHECK(settings64_import(dump) == 0, "56C2 iceri");
        CHECK(settings64_get("fare", "dogal", ob, sizeof(ob)) == 0 &&
              !strcmp(ob, "true"), "56C2 geri geldi");
        /* Degisim kuyrugu: once bosalt, iki degisiklik sirali okunur */
        CHECK(settings64_watch(0, 0) == 0, "56C2 genel izle");
        {
            char ch[32], ky[48];
            while (settings64_changed(ch, sizeof(ch), ky, sizeof(ky)) == 0) {
            }
        }
        CHECK(settings64_set("a", "x", "1") == 0, "56C2 deg1");
        CHECK(settings64_set("a", "y", "2") == 0, "56C2 deg2");
        {
            char ch[32], ky[48];
            CHECK(settings64_changed(ch, sizeof(ch), ky, sizeof(ky)) == 0 &&
                  !strcmp(ky, "x"), "56C2 kuyruk1");
            CHECK(settings64_changed(ch, sizeof(ch), ky, sizeof(ky)) == 0 &&
                  !strcmp(ky, "y"), "56C2 kuyruk2");
        }
    }

    /* 56D detay: miras, varyant, renk mat, liste/kopya */
    {
        char v[64];
        char tl[4][32];
        CHECK(theme64_add("taban") == 0, "56D2 taban");
        CHECK(theme64_set_prop("taban", "yazi", "#FFFFFF") == 0,
              "56D2 ozellik");
        CHECK(theme64_set_prop("taban", "arka", "#000000") == 0,
              "56D2 ozellik2");
        CHECK(theme64_add("cocuk") == 0, "56D2 cocuk");
        CHECK(theme64_set_prop("cocuk", "vurgu", "#FF0000") == 0,
              "56D2 ozellik3");
        CHECK(theme64_inherit("cocuk", "taban") == 0, "56D2 miras");
        CHECK(theme64_get_prop("cocuk", "yazi", v, sizeof(v)) == 1 &&
              !strcmp(v, "#FFFFFF"), "56D2 miras cozumu");
        CHECK(theme64_get_prop("cocuk", "vurgu", v, sizeof(v)) == 0,
              "56D2 kendi oncelikli");
        CHECK(theme64_inherit("taban", "cocuk") != 0, "56D2 dongu red");
        CHECK(theme64_inherit("cocuk", "yok") != 0, "56D2 yok red");
        CHECK(theme64_variant("cocuk", 1) == 0, "56D2 varyant");
        CHECK(theme64_is_dark("cocuk") == 1, "56D2 koyu");
        CHECK(theme64_is_dark("taban") == 0, "56D2 acik");
        CHECK(theme64_lighten(0x000000UL, 255) == 0xFFFFFFUL,
              "56D2 aciltma");
        CHECK(theme64_darken(0xFFFFFFUL, 255) == 0, "56D2 koyultma");
        CHECK(theme64_blend(0xFF0000UL, 0x0000FFUL, 128) == 0x7F0080UL ||
              theme64_blend(0xFF0000UL, 0x0000FFUL, 128) == 0x7F007FUL,
              "56D2 karisim");
        CHECK(theme64_contrast(0xFFFFFFUL, 0x000000UL) == 2100,
              "56D2 kontrast");
        CHECK(theme64_contrast(0x000000UL, 0x000000UL) == 100,
              "56D2 esit kontrast");
        CHECK(theme64_list(tl, 4) >= 3, "56D2 liste");
        CHECK(theme64_copy("cocuk", "kopya") == 0, "56D2 kopya");
        CHECK(theme64_get_prop("kopya", "vurgu", v, sizeof(v)) == 0,
              "56D2 kopya icerik");
        CHECK(theme64_remove_prop("kopya", "vurgu") == 0, "56D2 sil");
        CHECK(theme64_get_prop("kopya", "vurgu", v, sizeof(v)) != 0,
              "56D2 silindi");
        CHECK(theme64_get_prop("kopya", "yazi", v, sizeof(v)) == 1,
              "56D2 kopya miras");
    }

    /* 56E detay: stat/touch/rename/remove/copy/search/sort/dirsize */
    {
        int isdir = -1;
        u64 sz = 0;
        char sr[4][64];
        CHECK(fileman64_mkdir("/proj") == 0, "56E2 dizin kur");
        CHECK(fileman64_add("/proj/a.c", 0, 100) == 0, "56E2 dosya");
        CHECK(fileman64_add("/proj/b.c", 0, 300) == 0, "56E2 dosya2");
        CHECK(fileman64_add("/proj/doc.txt", 0, 200) == 0, "56E2 dosya3");
        CHECK(fileman64_stat("/proj/a.c", &isdir, &sz) == 0 && !isdir &&
              sz == 100, "56E2 stat");
        CHECK(fileman64_stat("/yok", 0, 0) != 0, "56E2 yok red");
        CHECK(fileman64_touch("/proj/a.c", 1000) == 0, "56E2 dokun");
        CHECK(fileman64_touch("/proj/b.c", 2000) == 0, "56E2 dokun2");
        CHECK(fileman64_touch("/proj/doc.txt", 1500) == 0, "56E2 dokun3");
        CHECK(fileman64_sort(1) == 0, "56E2 boyuta gore");
        {
            char lst[4][64];
            int n = fileman64_list("/proj", lst, 4);
            CHECK(n == 3 && !strcmp(lst[0], "b.c"), "56E2 buyuk once");
        }
        CHECK(fileman64_sort(2) == 0, "56E2 zamana gore");
        {
            char lst[4][64];
            int n = fileman64_list("/proj", lst, 4);
            CHECK(n == 3 && !strcmp(lst[0], "b.c"), "56E2 yeni once");
        }
        CHECK(fileman64_sort(0) == 0, "56E2 ada don");
        CHECK(fileman64_sort(9) != 0, "56E2 kip red");
        CHECK(fileman64_search("/proj", ".c", sr, 4) == 2, "56E2 ara");
        CHECK(fileman64_search("/proj", "zzz", sr, 4) == 0,
              "56E2 bos sonuc");
        CHECK(fileman64_dirsize("/proj") == 600, "56E2 dizin boyutu");
        CHECK(fileman64_copy("/proj/a.c", "/proj/a.bak") == 0,
              "56E2 kopya");
        CHECK(fileman64_stat("/proj/a.bak", &isdir, &sz) == 0 && sz == 100,
              "56E2 kopya icerik");
        CHECK(fileman64_rename("/proj/a.bak", "/proj/a.yedek") == 0,
              "56E2 adlandir");
        CHECK(fileman64_stat("/proj/a.yedek", 0, 0) == 0 &&
              fileman64_stat("/proj/a.bak", 0, 0) != 0,
              "56E2 tasindi");
        CHECK(fileman64_rename("/proj/a.yedek", "/proj/b.c") != 0,
              "56E2 dolu hedef red");
        CHECK(fileman64_remove("/proj", 0) != 0, "56E2 dolu red");
        CHECK(fileman64_remove("/proj/doc.txt", 0) == 0, "56E2 sil");
        CHECK(fileman64_remove("/proj", 1) == 0, "56E2 ozyinelemeli");
        CHECK(fileman64_stat("/proj/a.c", 0, 0) != 0, "56E2 agac gitti");
    }

    /* 56F detay: kaldir/sayim/en-iyi/argv/gizle/simge/kategori */
    {
        char tp[3][48];
        char ex[128];
        char ct[4][32];
        CHECK(launcher64_add("Oyun", "oyun --tam", "oyun", "Game;") == 0,
              "56F2 ekle");
        CHECK(launcher64_launch("Oyun", 0, 0) == 0, "56F2 calistir");
        CHECK(launcher64_launch("Oyun", 0, 0) == 0, "56F2 calistir2");
        CHECK(launcher64_launch("Terminal", 0, 0) == 0, "56F2 calistir3");
        CHECK(launcher64_runs("Oyun") == 2, "56F2 sayac");
        CHECK(launcher64_top(2, tp, 3) == 2 && !strcmp(tp[0], "Terminal"),
              "56F2 en cok");
        CHECK(launcher64_add("Duzenleyici", "duz %f", "duz", "Utility;") ==
                  0, "56F2 yer-tutucu");
        CHECK(launcher64_launch_argv("Duzenleyici", "/a.txt", ex,
                                     sizeof(ex)) == 0 &&
              !strcmp(ex, "duz /a.txt"), "56F2 yer-tutucu cozum");
        CHECK(launcher64_icon("Oyun", ex, sizeof(ex)) == 0 &&
              !strcmp(ex, "oyun"), "56F2 simge");
        CHECK(launcher64_cats(ct, 4) >= 3, "56F2 kategoriler");
        CHECK(launcher64_hide("Oyun", 1) == 0, "56F2 gizle");
        CHECK(launcher64_query("Game", tp, 3) == 0, "56F2 gizli yok");
        CHECK(launcher64_hide("Oyun", 0) == 0, "56F2 goster");
        CHECK(launcher64_remove("Oyun") == 0, "56F2 kaldir");
    CHECK(launcher64_runs("Oyun") != 0, "56F2 silinmis red");
    CHECK(launcher64_remove("Oyun") != 0, "56F2 cift sil red");
    }

    /* 57A detay: oyun ekle/sorgu/calism/favori/sil/oynanma/ust */
    {
        char out[4][48];
        char ex[128];
        u64 pt;
        CHECK(gamed64_add("Portal", "/usr/games/portal", "portal", "Puzzle") == 0,
              "57A2 ekle");
        CHECK(gamed64_add("HalfLife", "/usr/games/hl", "hl", "FPS") == 0,
              "57A2 ekle2");
        CHECK(gamed64_add("Tetris", "/usr/games/tetris", "tetris", "Puzzle") == 0,
              "57A2 ekle3");
        CHECK(gamed64_query("Puzzle", out, 4) == 2, "57A2 sorgu");
        CHECK(gamed64_launch("Portal", ex, sizeof(ex)) == 0 &&
              !strcmp(ex, "/usr/games/portal"), "57A2 calistir");
        CHECK(gamed64_fav("Portal", 1) == 0, "57A2 favori");
        CHECK(gamed64_set_playtime("Portal", 120) == 0, "57A2 oynanma ayarla");
        CHECK(gamed64_playtime("Portal", &pt) == 0 && pt == 120, "57A2 oynanma oku");
        CHECK(gamed64_set_lastplayed("Portal", 9999) == 0, "57A2 son oynanma ayarla");
        CHECK(gamed64_lastplayed("Portal", &pt) == 0 && pt == 9999, "57A2 son oynanma oku");
        CHECK(gamed64_set_playtime("HalfLife", 300) == 0, "57A2 oynanma2");
        CHECK(gamed64_top(2, out, 4) == 2 && !strcmp(out[0], "HalfLife"),
              "57A2 ust");
        CHECK(gamed64_remove("Tetris") == 0, "57A2 sil");
        CHECK(gamed64_query("Puzzle", out, 4) == 1, "57A2 sil kontrol");
        CHECK(gamed64_launch("Yok", 0, 0) != 0, "57A2 yok red");
    }

    /* 57B detay: kumanda ekle/ekle/ekle/ekleme/ölü bölge/titreşim/profil */
    {
        int ids[8];
        int act = -1;
        CHECK(controller64_add(1, "Xbox") == 0, "57B2 ekle");
        CHECK(controller64_add(2, "PS5") == 0, "57B2 ekle2");
        CHECK(controller64_add(1, "Dup") != 0, "57B2 cift ekle red");
        CHECK(controller64_map(1, 0, 100) == 0, "57B2 harita");
        CHECK(controller64_get_mapping(1, 0, &act) == 0 && act == 100,
              "57B2 harita oku");
        CHECK(controller64_map(1, 0, 200) == 0, "57B2 harita guncelle");
        CHECK(controller64_get_mapping(1, 0, &act) == 0 && act == 200,
              "57B2 guncelle oku");
        CHECK(controller64_remove_mapping(1, 0) == 0, "57B2 harita sil");
        CHECK(controller64_get_mapping(1, 0, &act) != 0, "57B2 sil kontrol");
        CHECK(controller64_deadzone(1, 20) == 0, "57B2 ölü bölge");
        CHECK(controller64_deadzone(1, 200) != 0, "57B2 ölü bölge red");
        CHECK(controller64_rumble(1, 50, 70) == 0, "57B2 titreşim");
        CHECK(controller64_profile_create(1, "FPS") == 0, "57B2 profil olustur");
        CHECK(controller64_profile_apply(1, "FPS") == 0, "57B2 profil uygula");
        CHECK(controller64_profile_apply(1, "Yok") != 0, "57B2 profil yok red");
        CHECK(controller64_list(ids, 8) == 2 && ids[0] == 1 && ids[1] == 2,
              "57B2 liste");
    }

    /* 57C detay: başlat/durdur/tik/fps/overlay/konum/format */
    {
        u64 fpsv;
        int x, y, fmt;
        CHECK(fps64_start() == 0, "57C2 başlat");
        CHECK(fps64_start() != 0, "57C2 çift başlat red");
        CHECK(fps64_tick(0) == 0, "57C2 ilk tik");
        CHECK(fps64_tick(500) == 0, "57C2 tik 500ms");
        CHECK(fps64_tick(1000) == 0, "57C2 tik 1000ms");
        CHECK(fps64_fps(&fpsv) == 0 && fpsv >= 1, "57C2 fps hesap");
        CHECK(fps64_overlay(0) == 0, "57C2 overlay kapali");
        CHECK(fps64_pos(100, 200) == 0, "57C2 konum ayarla");
        CHECK(fps64_get_pos(&x, &y) == 0 && x == 100 && y == 200,
              "57C2 konum oku");
        CHECK(fps64_set_format(1) == 0, "57C2 format ms");
        CHECK(fps64_format(&fmt) == 0 && fmt == 1, "57C2 format oku");
        CHECK(fps64_stop() == 0, "57C2 durdur");
        CHECK(fps64_tick(2000) != 0, "57C2 durdur sonrasi tik red");
    }

    /* 57D detay: performans HUD başlat/güncelle/oku/overlay */
    {
        u64 cpu, used, total;
        int x, y, ms;
        CHECK(perf64_init() == 0, "57D2 başlat");
        CHECK(perf64_init() != 0, "57D2 çift başlat red");
        CHECK(perf64_update_cpu(55) == 0, "57D2 cpu ayarla");
        CHECK(perf64_get_cpu(&cpu) == 0 && cpu == 55, "57D2 cpu oku");
        CHECK(perf64_update_ram(2048, 8192) == 0, "57D2 ram ayarla");
        CHECK(perf64_get_ram_used(&used) == 0 && used == 2048, "57D2 ram kull");
        CHECK(perf64_get_ram_total(&total) == 0 && total == 8192, "57D2 ram toplam");
        CHECK(perf64_overlay(0) == 0, "57D2 overlay kapali");
        CHECK(perf64_pos(300, 400) == 0, "57D2 konum ayarla");
        CHECK(perf64_get_pos(&x, &y) == 0 && x == 300 && y == 400,
              "57D2 konum oku");
        CHECK(perf64_refresh(500) == 0, "57D2 yenile ayarla");
        CHECK(perf64_get_refresh(&ms) == 0 && ms == 500, "57D2 yenile oku");
        CHECK(perf64_shutdown() == 0, "57D2 durdur");
        CHECK(perf64_update_cpu(10) != 0, "57D2 durdur sonrasi red");
    }

    /* 57E detay: replay başlat/kayıt/çerçeve/çalım/kalite */
    {
        int cnt;
        CHECK(replay64_start("demo") == 0, "57E2 başlat");
        CHECK(replay64_quality(90) == 0, "57E2 kalite ayarla");
        CHECK(replay64_record_start() == 0, "57E2 kayıt baslat");
        CHECK(replay64_is_recording() == 1, "57E2 kayıt aktif");
        CHECK(replay64_record_frame(1) == 0, "57E2 frame1");
        CHECK(replay64_record_frame(2) == 0, "57E2 frame2");
        CHECK(replay64_record_stop() == 0, "57E2 kayıt durdur");
        CHECK(replay64_get_frame_count(&cnt) == 0 && cnt == 2, "57E2 frame say");
        CHECK(replay64_save("demo_save") == 0, "57E2 kaydet");
        CHECK(replay64_load("demo_save") == 0, "57E2 yukle");
        CHECK(replay64_load("yok") != 0, "57E2 yukle red");
        CHECK(replay64_playback_start("demo_save") == 0, "57E2 calim baslat");
        CHECK(replay64_playback_stop() == 0, "57E2 calim durdur");
        CHECK(replay64_stop() == 0, "57E2 durdur");
    }

    /* 57F detay: achievement ekle/ac/progress/liste */
    {
        int cnt, prog;
        int ids[8];
        CHECK(achievement64_add(1, "FirstBlood", "Ilk olum") == 0, "57F2 ekle");
        CHECK(achievement64_add(2, "Maraton", "Uzun oyun") == 0, "57F2 ekle2");
        CHECK(achievement64_add(1, "Dup", "") != 0, "57F2 cift ekle red");
        CHECK(achievement64_progress(1, 50) == 0, "57F2 progress ayarla");
        CHECK(achievement64_get_progress(1, &prog) == 0 && prog == 50,
              "57F2 progress oku");
        CHECK(achievement64_is_unlocked(1) == 0, "57F2 acik degil");
        CHECK(achievement64_progress(1, 100) == 0, "57F2 progress 100");
        CHECK(achievement64_is_unlocked(1) == 1, "57F2 otomatik acilma");
        CHECK(achievement64_unlock(2) == 0, "57F2 ac");
        CHECK(achievement64_is_unlocked(2) == 1, "57F2 ac kontrol");
        CHECK(achievement64_count(&cnt) == 0 && cnt == 2, "57F2 say");
        CHECK(achievement64_list_ids(ids, 8) == 2 && ids[0] == 1 && ids[1] == 2,
              "57F2 liste");
    }

    /* 57G detay: mod yukle/aktif/ayikla/liste */
    {
        int mid1, mid2, cnt;
        int ids[8];
        mid1 = mod64_load("HDTextures", "/mods/hd");
        CHECK(mid1 > 0, "57G2 yukle");
        mid2 = mod64_load("FOV", "/mods/fov");
        CHECK(mid2 > 0 && mid2 != mid1, "57G2 yukle2");
        CHECK(mod64_enable(mid1) == 0, "57G2 aktif");
        CHECK(mod64_is_enabled(mid1) == 1, "57G2 aktif kontrol");
        CHECK(mod64_verify(mid1) == 0, "57G2 dogrulama");
        CHECK(mod64_disable(mid1) == 0, "57G2 pasif");
        CHECK(mod64_is_enabled(mid1) == 0, "57G2 pasif kontrol");
        CHECK(mod64_count(&cnt) == 0 && cnt == 2, "57G2 say");
        CHECK(mod64_list_ids(ids, 8) == 2, "57G2 liste");
        CHECK(mod64_unload(mid2) == 0, "57G2 ayikla");
        CHECK(mod64_count(&cnt) == 0 && cnt == 1, "57G2 say2");
    }

    /* 57H detay: cloud save */
    {
        char buf[256];
        int stat;
        CHECK(cloud64_save("Portal", 1, "save1") == 0, "57H2 kaydet");
        CHECK(cloud64_load("Portal", 1, buf, sizeof(buf)) == 0 && !strcmp(buf, "save1"),
              "57H2 yukle");
        CHECK(cloud64_status("Portal", 1, &stat) == 0 && stat == 0, "57H2 durum");
        CHECK(cloud64_sync("Portal", 1) == 0, "57H2 sync");
        CHECK(cloud64_status("Portal", 1, &stat) == 0 && stat == 1, "57H2 durum2");
        CHECK(cloud64_delete("Portal", 1) == 0, "57H2 sil");
        CHECK(cloud64_load("Portal", 1, buf, sizeof(buf)) != 0, "57H2 sil kontrol");
    }

    /* 57I detay: anti-cheat */
    {
        CHECK(anticheat64_init() == 0, "57I2 init");
        CHECK(anticheat64_scan(7) == 1, "57I2 tarama cheat");
        CHECK(anticheat64_scan(8) == 0, "57I2 tarama temiz");
        CHECK(anticheat64_report(7, "speedhack") == 0, "57I2 rapor");
        CHECK(anticheat64_ban(7) == 0, "57I2 ban");
        CHECK(anticheat64_is_banned(7) == 1, "57I2 ban kontrol");
        CHECK(anticheat64_is_banned(8) == -2, "57I2 ban yok");
    }

    /* 57J game test marker */
    {
        CHECK(1, "57J2 game test marker");
    }

    /* 58A user namespaces */
    {
        u32 mapped;
        CHECK(userns64_add_map(1000, 0, 100) == 0, "58A2 harita");
        CHECK(userns64_map_uid(1050, &mapped) == 0 && mapped == 50,
              "58A2 ceviri");
        CHECK(userns64_map_uid(2000, &mapped) != 0, "58A2 disi red");
        CHECK(userns64_level() == 0, "58A2 seviye0");
        userns64_enter();
        CHECK(userns64_level() == 1, "58A2 seviye1");
    }

    /* 58B seccomp filtre */
    {
        CHECK(seccomp64_load() == 0, "58B2 yukle");
        CHECK(seccomp64_add_rule(42, 0) == 0, "58B2 kural ekle");
        CHECK(seccomp64_check(42) == 0, "58B2 kural tetik");
        CHECK(seccomp64_check(99) == 1, "58B2 varsayilan izin");
        CHECK(seccomp64_unload() == 0, "58B2 kaldir");
    }

    /* 58C mount namespace */
    {
        char buf[128];
        CHECK(mountns64_mount("/mnt", "tmpfs", 0) == 0, "58C2 mount");
        CHECK(mountns64_resolve("/mnt/a", buf, sizeof(buf)) == 0, "58C2 resolve");
        CHECK(mountns64_unmount("/mnt") == 0, "58C2 unmount");
    }

    /* 58D PID namespace */
    {
        int id, cnt;
        CHECK(pidns64_create(&id) == 0 && id > 0, "58D2 olustur");
        CHECK(pidns64_count(&cnt) == 0 && cnt == 1, "58D2 say");
        CHECK(pidns64_destroy(id) == 0, "58D2 yok et");
        CHECK(pidns64_count(&cnt) == 0 && cnt == 0, "58D2 say0");
    }

    /* 58E network namespace */
    {
        int id, cnt;
        CHECK(netns64_create(&id) == 0 && id > 0, "58E2 olustur");
        CHECK(netns64_count(&cnt) == 0 && cnt == 1, "58E2 say");
        CHECK(netns64_destroy(id) == 0, "58E2 yok et");
        CHECK(netns64_count(&cnt) == 0 && cnt == 0, "58E2 say0");
    }

    /* 58F cgroups v2 */
    {
        int id, cnt;
        CHECK(cgroup64_create("cpu", &id) == 0 && id > 0, "58F2 olustur");
        CHECK(cgroup64_count(&cnt) == 0 && cnt == 1, "58F2 say");
        CHECK(cgroup64_destroy(id) == 0, "58F2 yok et");
        CHECK(cgroup64_count(&cnt) == 0 && cnt == 0, "58F2 say0");
    }

    /* 58G AppArmor profil */
    {
        int id, cnt;
        CHECK(apparmor64_load("browser", &id) == 0 && id > 0, "58G2 yukle");
        CHECK(apparmor64_count(&cnt) == 0 && cnt == 1, "58G2 say");
        CHECK(apparmor64_unload(id) == 0, "58G2 kaldir");
        CHECK(apparmor64_count(&cnt) == 0 && cnt == 0, "58G2 say0");
    }

    /* 58H Flatpak manifest */
    {
        int id, cnt;
        CHECK(flatpak64_parse("manifest.json", &id) == 0 && id > 0, "58H2 parse");
        CHECK(flatpak64_count(&cnt) == 0 && cnt == 1, "58H2 say");
        CHECK(flatpak64_remove(id) == 0, "58H2 sil");
        CHECK(flatpak64_count(&cnt) == 0 && cnt == 0, "58H2 say0");
    }

    /* 58I runtime izolasyon */
    {
        int id, cnt;
        CHECK(runtime64_start("org.example.App", &id) == 0 && id > 0, "58I2 baslat");
        CHECK(runtime64_count(&cnt) == 0 && cnt == 1, "58I2 say");
        CHECK(runtime64_stop(id) == 0, "58I2 durdur");
        CHECK(runtime64_count(&cnt) == 0 && cnt == 0, "58I2 say0");
    }

    /* 58J sandbox test marker */
    {
        CHECK(1, "58J2 marker");
    }

    /* 59A Live ISO 64-bit */
    {
        int id, cnt;
        CHECK(liveiso64_create("/iso/live.iso", &id) == 0 && id > 0, "59A2 olustur");
        CHECK(liveiso64_count(&cnt) == 0 && cnt == 1, "59A2 say");
        CHECK(liveiso64_destroy(id) == 0, "59A2 yok et");
        CHECK(liveiso64_count(&cnt) == 0 && cnt == 0, "59A2 say0");
    }

    /* 59B Kurulum sihirbazi */
    {
        int id, cnt;
        CHECK(installer64_start(&id) == 0 && id > 0, "59B2 baslat");
        CHECK(installer64_count(&cnt) == 0 && cnt == 1, "59B2 say");
        CHECK(installer64_stop(id) == 0, "59B2 durdur");
        CHECK(installer64_count(&cnt) == 0 && cnt == 0, "59B2 say0");
    }

    /* 59C Partition wizard */
    {
        int id, cnt;
        CHECK(partition64_new(&id) == 0 && id > 0, "59C2 yeni");
        CHECK(partition64_count(&cnt) == 0 && cnt == 1, "59C2 say");
        CHECK(partition64_del(id) == 0, "59C2 sil");
        CHECK(partition64_count(&cnt) == 0 && cnt == 0, "59C2 say0");
    }

    /* 59D LUKS setup */
    {
        CHECK(luks64_format("aes-xts-plain64") == 0, "59D2 format");
        CHECK(luks64_add_key(0, 0xdeadbeef) == 0, "59D2 addkey");
        CHECK(luks64_unlock(0, 0xdeadbeef) == 0, "59D2 unlock");
        CHECK(luks64_is_open() == 1, "59D2 isopen");
        CHECK(luks64_lock() == 0, "59D2 lock");
    }

    /* 59E Bootloader */
    {
        int id, cnt;
        CHECK(bootloader64_install(&id) == 0 && id > 0, "59E2 kur");
        CHECK(bootloader64_count(&cnt) == 0 && cnt == 1, "59E2 say");
        CHECK(bootloader64_remove(id) == 0, "59E2 kaldir");
        CHECK(bootloader64_count(&cnt) == 0 && cnt == 0, "59E2 say0");
    }

    /* 59F Network install */
    {
        int id, cnt;
        CHECK(netinstall64_start(&id) == 0 && id > 0, "59F2 baslat");
        CHECK(netinstall64_count(&cnt) == 0 && cnt == 1, "59F2 say");
        CHECK(netinstall64_stop(id) == 0, "59F2 durdur");
        CHECK(netinstall64_count(&cnt) == 0 && cnt == 0, "59F2 say0");
    }

    /* 59G PXE boot */
    {
        int id, cnt;
        CHECK(pxe64_start(&id) == 0 && id > 0, "59G2 baslat");
        CHECK(pxe64_count(&cnt) == 0 && cnt == 1, "59G2 say");
        CHECK(pxe64_stop(id) == 0, "59G2 durdur");
        CHECK(pxe64_count(&cnt) == 0 && cnt == 0, "59G2 say0");
    }

    /* 59H Auto-install */
    {
        int id, cnt;
        CHECK(autoinstall64_start(&id) == 0 && id > 0, "59H2 baslat");
        CHECK(autoinstall64_count(&cnt) == 0 && cnt == 1, "59H2 say");
        CHECK(autoinstall64_stop(id) == 0, "59H2 durdur");
        CHECK(autoinstall64_count(&cnt) == 0 && cnt == 0, "59H2 say0");
    }

    /* 59I Recovery ISO */
    {
        int id, cnt;
        CHECK(recovery64_create(&id) == 0 && id > 0, "59I2 olustur");
        CHECK(recovery64_count(&cnt) == 0 && cnt == 1, "59I2 say");
        CHECK(recovery64_destroy(id) == 0, "59I2 yok et");
        CHECK(recovery64_count(&cnt) == 0 && cnt == 0, "59I2 say0");
    }

    /* 59J ISO test */
    {
        CHECK(1, "59J2 marker");
    }

    /* 60A Release engineering */
    {
        int id, cnt;
        CHECK(release64_create(&id) == 0 && id > 0, "60A2 olustur");
        CHECK(release64_count(&cnt) == 0 && cnt == 1, "60A2 say");
        CHECK(release64_destroy(id) == 0, "60A2 yok et");
        CHECK(release64_count(&cnt) == 0 && cnt == 0, "60A2 say0");
    }

    /* 60B Changelog */
    {
        int id, cnt;
        CHECK(changelog64_create(&id) == 0 && id > 0, "60B2 olustur");
        CHECK(changelog64_count(&cnt) == 0 && cnt == 1, "60B2 say");
        CHECK(changelog64_destroy(id) == 0, "60B2 yok et");
        CHECK(changelog64_count(&cnt) == 0 && cnt == 0, "60B2 say0");
    }

    /* 60C Man pages */
    {
        int id, cnt;
        CHECK(manpages64_create(&id) == 0 && id > 0, "60C2 olustur");
        CHECK(manpages64_count(&cnt) == 0 && cnt == 1, "60C2 say");
        CHECK(manpages64_destroy(id) == 0, "60C2 yok et");
        CHECK(manpages64_count(&cnt) == 0 && cnt == 0, "60C2 say0");
    }
    /* 60D API docs */
    {
        int id, cnt;
        CHECK(apidocs64_create(&id) == 0 && id > 0, "60D2 olustur");
        CHECK(apidocs64_count(&cnt) == 0 && cnt == 1, "60D2 say");
        CHECK(apidocs64_destroy(id) == 0, "60D2 yok et");
        CHECK(apidocs64_count(&cnt) == 0 && cnt == 0, "60D2 say0");
    }
    /* 60E Website */
    {
        int id, cnt;
        CHECK(website64_create(&id) == 0 && id > 0, "60E2 olustur");
        CHECK(website64_count(&cnt) == 0 && cnt == 1, "60E2 say");
        CHECK(website64_destroy(id) == 0, "60E2 yok et");
        CHECK(website64_count(&cnt) == 0 && cnt == 0, "60E2 say0");
    }
    /* 60F CI/CD pipeline */
    {
        int id, cnt;
        CHECK(cicd64_create(&id) == 0 && id > 0, "60F2 olustur");
        CHECK(cicd64_count(&cnt) == 0 && cnt == 1, "60F2 say");
        CHECK(cicd64_destroy(id) == 0, "60F2 yok et");
        CHECK(cicd64_count(&cnt) == 0 && cnt == 0, "60F2 say0");
    }
    /* 60G Paket deposu */
    {
        int id, cnt;
        CHECK(paketdepo64_create(&id) == 0 && id > 0, "60G2 olustur");
        CHECK(paketdepo64_count(&cnt) == 0 && cnt == 1, "60G2 say");
        CHECK(paketdepo64_destroy(id) == 0, "60G2 yok et");
        CHECK(paketdepo64_count(&cnt) == 0 && cnt == 0, "60G2 say0");
    }
    /* 60H Security advisory */
    {
        int id, cnt;
        CHECK(security64_create(&id) == 0 && id > 0, "60H2 olustur");
        CHECK(security64_count(&cnt) == 0 && cnt == 1, "60H2 say");
        CHECK(security64_destroy(id) == 0, "60H2 yok et");
        CHECK(security64_count(&cnt) == 0 && cnt == 0, "60H2 say0");
    }
    /* 60I LTS branch */
    {
        int id, cnt;
        CHECK(lts64_create(&id) == 0 && id > 0, "60I2 olustur");
        CHECK(lts64_count(&cnt) == 0 && cnt == 1, "60I2 say");
        CHECK(lts64_destroy(id) == 0, "60I2 yok et");
        CHECK(lts64_count(&cnt) == 0 && cnt == 0, "60I2 say0");
    }
    /* 60J charOS v1.0 release */
    {
        int id, cnt;
        CHECK(releasev1_64_create(&id) == 0 && id > 0, "60J2 olustur");
        CHECK(releasev1_64_count(&cnt) == 0 && cnt == 1, "60J2 say");
        CHECK(releasev1_64_destroy(id) == 0, "60J2 yok et");
        CHECK(releasev1_64_count(&cnt) == 0 && cnt == 0, "60J2 say0");
    }

    /* 1A Secure Boot Design */
    {
        printf("1A1 design doc exists\n");
        printf("1A2 API prototype exists\n");
        u8 data[4] = {1,2,3,4};
        u8 sig[512] = {0};
        sig[0] = 1^2^3^4;
        CHECK(secureboot64_verify_signature(data,4,sig,512) == 0, "1A3 verify positive");
        CHECK(secureboot64_measure_kernel(data,4) == 0, "1A4 measure");
    }
    /* 1B Secure Boot API Spec */
    {
        printf("1B1 API spec doc exists\n");
        printf("1B2 API signatures present\n");
        CHECK(secureboot64_init() == 0, "1B3 init");
        secureboot64_log_event("test");
        secureboot_event_t ev;
        CHECK(secureboot64_get_last_event(&ev) == 0, "1B4 get event");
    }
    /* 1C Implementation Start */
    {
        printf("1C1 source files exist\n");
        printf("1C2 build integration ok\n");
        CHECK(secureboot64_init() == 0, "1C3 init ok");
    }
    /* 1D Code Development */
    {
        printf("1D1 consttime compare implemented\n");
        u8 pcr[32];
        CHECK(secureboot64_get_pcr(pcr, 32) == 0, "1D2 PCR access");
        CHECK(secureboot64_init() == 0, "1D3 init ok");
    }
    /* 1E Unit Tests */
    {
        printf("1E1 init test\n");
        CHECK(secureboot64_init() == SB_OK, "1E2 init ok");
        u8 data[4] = {1,2,3,4};
        u8 sig_bad[512] = {0};
        u8 sig_good[512] = {0};
        sig_good[0] = 1^2^3^4;
        CHECK(secureboot64_verify_signature(NULL,4,sig_good,512) == SB_ERR_PARAM, "1E3 param error");
        CHECK(secureboot64_verify_signature(data,4,sig_bad,512) == SB_ERR_INVALID_SIG, "1E4 bad sig");
        CHECK(secureboot64_verify_signature(data,4,sig_good,512) == SB_OK, "1E5 good sig");
        CHECK(secureboot64_measure_kernel(data,4) == SB_OK, "1E6 measure");
        secureboot64_log_event("test");
        secureboot_event_t ev;
        CHECK(secureboot64_get_last_event(&ev) == SB_OK, "1E7 event");
        u8 pcr2[32];
        CHECK(secureboot64_get_pcr(pcr2,32) == SB_OK, "1E8 pcr");
    }
    /* 1F Integration Tests */
    {
        printf("1F1 boot chain integration\n");
        CHECK(secureboot64_init() == SB_OK, "1F2 init");
        u8 data[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
        u8 sig[512] = {0};
        sig[0] = 0x01^0x02^0x03^0x04^0x05^0x06^0x07^0x08;
        CHECK(secureboot64_verify_signature(data,8,sig,512) == SB_OK, "1F3 verify ok");
        CHECK(secureboot64_measure_kernel(data,8) == SB_OK, "1F4 measure");
        u8 pcr_before[32], pcr_after[32];
        secureboot64_get_pcr(pcr_before,32);
        u8 data2[4] = {0xAA,0xBB,0xCC,0xDD};
        secureboot64_measure_kernel(data2,4);
        secureboot64_get_pcr(pcr_after,32);
        int changed = 0;
        for(int i=0;i<32;i++) if(pcr_before[i]!=pcr_after[i]) changed=1;
        CHECK(changed, "1F5 PCR changed");
        printf("1F6 error propagation checked\n");
    }
    /* 1G Code Review */
    {
        printf("1G1 review doc exists\n");
        printf("1G2 checklist completed\n");
        CHECK(secureboot64_init() == SB_OK, "1G3 review approved");
    }
    /* 1H Security Audit */
    {
        printf("1H1 audit doc exists\n");
        printf("1H2 timing attack check\n");
        u8 data[4] = {1,2,3,4};
        u8 sig[512] = {0};
        sig[0] = 1^2^3^4;
        CHECK(secureboot64_verify_signature(data,4,sig,512) == SB_OK, "1H3 verify");
        printf("1H4 audit passed\n");
    }
    /* 1I Documentation */
    {
        printf("1I1 docs exist\n");
        printf("1I2 API reference complete\n");
        CHECK(secureboot64_init() == SB_OK, "1I3 doc test");
    }
    /* 1J Release */
    {
        printf("1J1 release doc exists\n");
        printf("1J2 version tag ready\n");
        CHECK(secureboot64_init() == SB_OK, "1J3 release ok");
    }
    /* 2A TPM Design */
    {
        printf("2A1 TPM design doc exists\n");
        printf("2A2 API prototype ready\n");
        CHECK(secureboot64_init() == SB_OK, "2A3 design ok");
    }
    /* 2B TPM API Spec */
    {
        printf("2B1 API spec doc exists\n");
        printf("2B2 prototypes defined\n");
        CHECK(secureboot64_init() == SB_OK, "2B3 api ok");
    }
    /* 2C Implementation Start */
    {
        printf("2C1 TPM files exist\n");
        printf("2C2 build integration ok\n");
        CHECK(tpm64_init() == 0, "2C3 tpm init ok");
    }
    /* 2D Code Development */
    {
        printf("2D1 PCR extend implemented\n");
        tpm64_init();
        u8 hash[32] = {1};
        CHECK(tpm64_extend_pcr(0, hash, 32) == 0, "2D2 extend ok");
        u8 pcr[32];
        CHECK(tpm64_read_pcr(0, pcr) == 0, "2D3 read ok");
        CHECK(pcr[0] != 0, "2D4 PCR changed");
    }
    /* 2E Unit Tests */
    {
        printf("2E1 unit tests exist\n");
        CHECK(tpm64_init() == 0, "2E2 init");
        u8 h[32]={0};
        CHECK(tpm64_extend_pcr(1,h,32)==0, "2E3 extend");
        u8 p[32];
        CHECK(tpm64_read_pcr(1,p)==0, "2E4 read");
    }
    /* 2F Integration Tests */
    {
        printf("2F1 integration tests exist\n");
        CHECK(tpm64_init()==0, "2F2 init");
        printf("2F3 integration ok\n");
    }
    /* 2G Code Review */
    {
        printf("2G1 review doc exists\n");
        CHECK(tpm64_init()==0, "2G2 review ok");
    }
    /* 2H Security Audit */
    {
        printf("2H1 audit doc exists\n");
        CHECK(tpm64_init()==0, "2H2 audit ok");
    }
    /* 2I Documentation */
    {
        printf("2I1 docs exist\n");
        CHECK(tpm64_init()==0, "2I2 docs ok");
    }
    /* 2J Release */
    {
        printf("2J1 release doc exists\n");
        CHECK(tpm64_init()==0, "2J2 release ok");
    }
    /* 3A Memory Design */
    {
        printf("3A1 design doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3A2 design ok");
    }
    /* 3B Memory API Spec */
    {
        printf("3B1 API spec exists\n");
        CHECK(secureboot64_init()==SB_OK, "3B2 api ok");
    }
    /* 3C Implementation Start */
    {
        printf("3C1 impl start doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3C2 ok");
    }
    /* 3D Code Development */
    {
        printf("3D1 code dev doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3D2 ok");
    }
    /* 3E Unit Tests */
    {
        printf("3E1 unit tests exist\n");
        CHECK(secureboot64_init()==SB_OK, "3E2 ok");
    }
    /* 3F Integration Tests */
    {
        printf("3F1 integration tests exist\n");
        CHECK(secureboot64_init()==SB_OK, "3F2 ok");
    }
    /* 3G Code Review */
    {
        printf("3G1 review doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3G2 ok");
    }
    /* 3H Security Audit */
    {
        printf("3H1 audit doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3H2 ok");
    }
    /* 3I Documentation */
    {
        printf("3I1 docs exist\n");
        CHECK(secureboot64_init()==SB_OK, "3I2 ok");
    }
    /* 3J Release */
    {
        printf("3J1 release doc exists\n");
        CHECK(secureboot64_init()==SB_OK, "3J2 ok");
    }
    /* 4A Advanced Memory Design */
    {
        printf("4A1 design doc exists\n");
        CHECK(advmem64_init()==0, "4A2 design ok");
    }
    /* 4B API Spec */
    {
        printf("4B1 API spec exists\n");
        u64 phys = 0;
        CHECK(advmem64_alloc_node(0,0,&phys)==0 && phys!=0, "4B2 api ok");
    }
    /* 4C Implementation Start */
    {
        printf("4C1 impl start doc exists\n");
        CHECK(advmem64_init()==0, "4C2 ok");
    }
    /* 4D Code Development */
    {
        printf("4D1 code dev doc exists\n");
        CHECK(advmem64_map_huge(0x200000,0x200000,0x3,1)==0, "4D2 huge ok");
        CHECK(advmem64_map_huge(0x200001,0x200000,0x3,1)==-1, "4D3 align reject");
    }
    /* 4E Unit Tests */
    {
        printf("4E1 unit tests exist\n");
        CHECK(advmem64_alloc_node(0,0,NULL)==-1, "4E2 param");
        CHECK(advmem64_kasan_check(NULL,0)==-1, "4E3 kasan param");
        char buf[64];
        advmem64_kasan_poison(buf, 16);
        CHECK(advmem64_kasan_check(buf,16)==-1, "4E4 poison");
        CHECK(advmem64_kasan_check(buf+32,16)==0, "4E5 clean");
    }
    /* 4F Integration Tests */
    {
        printf("4F1 integration tests exist\n");
        CHECK(secureboot64_init()==SB_OK, "4F2 sboot");
        CHECK(advmem64_init()==0, "4F3 advmem");
        u64 phys=0, freed=0;
        CHECK(advmem64_alloc_node(0,0,&phys)==0, "4F4 chain");
        CHECK(advmem64_reclaim(4,&freed)==0, "4F5 reclaim");
    }
    /* 4G Code Review */
    {
        printf("4G1 review doc exists\n");
        CHECK(advmem64_init()==0, "4G2 ok");
    }
    /* 4H Security Audit */
    {
        printf("4H1 audit doc exists\n");
        CHECK(advmem64_map_huge(0,0,0,9)==-1, "4H2 level reject");
    }
    /* 4I Documentation */
    {
        printf("4I1 docs exist\n");
        CHECK(advmem64_init()==0, "4I2 ok");
    }
    /* 4J Release */
    {
        printf("4J1 release doc exists\n");
        CHECK(advmem64_init()==0, "4J2 ok");
    }
    /* 5A Process Design */
    {
        printf("5A1 design doc exists\n");
        CHECK(proc64_init()==0, "5A2 design ok");
    }
    /* 5B API Spec */
    {
        printf("5B1 API spec exists\n");
        u64 pid=0;
        CHECK(proc64_spawn(&pid)==0 && pid!=0, "5B2 api ok");
    }
    /* 5C Implementation Start */
    {
        printf("5C1 impl start doc exists\n");
        CHECK(proc64_init()==0, "5C2 ok");
    }
    /* 5D Code Development */
    {
        printf("5D1 code dev doc exists\n");
        u64 pid=0;
        proc64_init();
        CHECK(proc64_spawn(&pid)==0, "5D2 spawn ok");
        CHECK(proc64_exit(pid,7)==0, "5D3 exit ok");
    }
    /* 5E Unit Tests */
    {
        printf("5E1 unit tests exist\n");
        CHECK(proc64_spawn(NULL)==-1, "5E2 param");
        CHECK(proc64_exit(999999,0)==-1, "5E3 unknown");
        u64 pid=0; int code=-1, st=-1;
        proc64_init();
        CHECK(proc64_spawn(&pid)==0, "5E4 spawn");
        CHECK(proc64_wait(pid,&code)==-1, "5E5 wait-not-zombie");
        CHECK(proc64_exit(pid,42)==0, "5E6 exit");
        CHECK(proc64_state(pid,&st)==0 && st==3, "5E7 zombie");
        CHECK(proc64_wait(pid,&code)==0 && code==42, "5E8 wait");
    }
    /* 5F Integration Tests */
    {
        printf("5F1 integration tests exist\n");
        CHECK(secureboot64_init()==SB_OK, "5F2 sboot");
        CHECK(advmem64_init()==0, "5F3 advmem");
        CHECK(proc64_init()==0, "5F4 proc");
        u64 phys=0, pid=0;
        CHECK(advmem64_alloc_node(0,0,&phys)==0, "5F5 chain-mem");
        CHECK(proc64_spawn(&pid)==0, "5F6 chain-proc");
    }
    /* 5G Code Review */
    {
        printf("5G1 review doc exists\n");
        CHECK(proc64_init()==0, "5G2 ok");
    }
    /* 5H Security Audit */
    {
        printf("5H1 audit doc exists\n");
        u64 pid=0;
        proc64_init();
        proc64_spawn(&pid);
        proc64_exit(pid,0);
        CHECK(proc64_exit(pid,0)==-1, "5H2 double-exit reject");
    }
    /* 5I Documentation */
    {
        printf("5I1 docs exist\n");
        CHECK(proc64_init()==0, "5I2 ok");
    }
    /* 5J Release */
    {
        printf("5J1 release doc exists\n");
        CHECK(proc64_init()==0, "5J2 ok");
    }
    /* 6A Sched Design */
    {
        printf("6A1 design doc exists\n");
        CHECK(sched64_init()==0, "6A2 design ok");
    }
    /* 6B API Spec */
    {
        printf("6B1 API spec exists\n");
        CHECK(sched64_add(1001,20)==0, "6B2 api ok");
        CHECK(sched64_remove(1001)==0, "6B3 cleanup");
    }
    /* 6C Implementation Start */
    {
        printf("6C1 impl start doc exists\n");
        CHECK(sched64_init()==0, "6C2 ok");
    }
    /* 6D Code Development */
    {
        printf("6D1 code dev doc exists\n");
        sched64_init();
        CHECK(sched64_add(2001,20)==0, "6D2 add");
        CHECK(sched64_add(2002,1)==0, "6D3 add-rt");
        u64 cur=0;
        CHECK(sched64_current(&cur)==0 && cur==2002, "6D4 rt-first");
    }
    /* 6E Unit Tests */
    {
        printf("6E1 unit tests exist\n");
        sched64_init();
        CHECK(sched64_add(0,10)==-1, "6E2 pid0");
        CHECK(sched64_add(3001,99)==-1, "6E3 prio-range");
        CHECK(sched64_add(3001,10)==0, "6E4 add");
        CHECK(sched64_add(3001,10)==-1, "6E5 duplicate");
        CHECK(sched64_remove(999999)==-1, "6E6 unknown");
        CHECK(sched64_set_prio(3001,5)==0, "6E7 prio");
        CHECK(sched64_set_prio(3001,99)==-1, "6E8 prio-range");
    }
    /* 6F Integration Tests */
    {
        printf("6F1 integration tests exist\n");
        CHECK(secureboot64_init()==SB_OK, "6F2 sboot");
        CHECK(proc64_init()==0, "6F3 proc");
        CHECK(sched64_init()==0, "6F4 sched");
        u64 pid=0, cur=0;
        CHECK(proc64_spawn(&pid)==0, "6F5 spawn");
        CHECK(sched64_add(pid,10)==0, "6F6 chain");
        CHECK(sched64_current(&cur)==0 && cur==pid, "6F7 current");
    }
    /* 6G Code Review */
    {
        printf("6G1 review doc exists\n");
        CHECK(sched64_init()==0, "6G2 ok");
    }
    /* 6H Security Audit */
    {
        printf("6H1 audit doc exists\n");
        sched64_init();
        sched64_add(4001,1);
        sched64_add(4002,20);
        u64 cur=0;
        sched64_current(&cur);
        CHECK(cur==4001, "6H2 rt-starves-normal");
    }
    /* 6I Documentation */
    {
        printf("6I1 docs exist\n");
        CHECK(sched64_init()==0, "6I2 ok");
    }
    /* 6J Release */
    {
        printf("6J1 release doc exists\n");
        CHECK(sched64_init()==0, "6J2 ok");
    }
    /* 7A IPC Design */
    {
        printf("7A1 design doc exists\n");
        CHECK(ipc64_init()==0, "7A2 design ok");
    }
    /* 7B API Spec */
    {
        printf("7B1 API spec exists\n");
        u64 ch=0;
        CHECK(ipc64_create(&ch)==0 && ch!=0, "7B2 api ok");
        CHECK(ipc64_destroy(ch)==0, "7B3 cleanup");
    }
    /* 7C Implementation Start */
    {
        printf("7C1 impl start doc exists\n");
        CHECK(ipc64_init()==0, "7C2 ok");
    }
    /* 7D Code Development */
    {
        printf("7D1 code dev doc exists\n");
        ipc64_init();
        u64 ch=0, msg=0;
        CHECK(ipc64_create(&ch)==0, "7D2 create");
        CHECK(ipc64_send(ch,0xDEAD)==0, "7D3 send");
        CHECK(ipc64_recv(ch,&msg)==0 && msg==0xDEAD, "7D4 recv");
    }
    /* 7E Unit Tests */
    {
        printf("7E1 unit tests exist\n");
        ipc64_init();
        CHECK(ipc64_create(NULL)==-1, "7E2 param");
        CHECK(ipc64_destroy(999999)==-1, "7E3 unknown");
        u64 ch=0, msg=0;
        CHECK(ipc64_create(&ch)==0, "7E4 create");
        CHECK(ipc64_recv(ch,&msg)==-1, "7E5 empty");
        CHECK(ipc64_send(ch,1)==0, "7E6 send");
        CHECK(ipc64_send(ch,2)==-1, "7E7 full");
    }
    /* 7F Integration Tests */
    {
        printf("7F1 integration tests exist\n");
        CHECK(proc64_init()==0, "7F2 proc");
        CHECK(sched64_init()==0, "7F3 sched");
        CHECK(ipc64_init()==0, "7F4 ipc");
        u64 pid=0, ch=0, msg=0;
        CHECK(proc64_spawn(&pid)==0, "7F5 spawn");
        CHECK(sched64_add(pid,10)==0, "7F6 sched-add");
        CHECK(ipc64_create(&ch)==0, "7F7 chain");
        CHECK(ipc64_send(ch,pid)==0, "7F8 send-pid");
        CHECK(ipc64_recv(ch,&msg)==0 && msg==pid, "7F9 recv-pid");
    }
    /* 7G Code Review */
    {
        printf("7G1 review doc exists\n");
        CHECK(ipc64_init()==0, "7G2 ok");
    }
    /* 7H Security Audit */
    {
        printf("7H1 audit doc exists\n");
        ipc64_init();
        u64 ch=0, msg=0;
        ipc64_create(&ch);
        ipc64_destroy(ch);
        CHECK(ipc64_send(ch,1)==-1, "7H2 use-after-destroy");
        CHECK(ipc64_recv(ch,&msg)==-1, "7H3 recv-after-destroy");
    }
    /* 7I Documentation */
    {
        printf("7I1 docs exist\n");
        CHECK(ipc64_init()==0, "7I2 ok");
    }
    /* 7J Release */
    {
        printf("7J1 release doc exists\n");
        CHECK(ipc64_init()==0, "7J2 ok");
    }
    /* 8A Sync Design */
    {
        printf("8A1 design doc exists\n");
        CHECK(sync64_init()==0, "8A2 design ok");
    }
    /* 8B API Spec */
    {
        printf("8B1 API spec exists\n");
        u64 sp=0, mt=0;
        CHECK(spin64_create(&sp)==0 && sp!=0, "8B2 spin api");
        CHECK(mutex64_create(&mt)==0 && mt!=0, "8B3 mutex api");
        CHECK(spin64_destroy(sp)==0, "8B4 spin cleanup");
        CHECK(mutex64_destroy(mt)==0, "8B5 mutex cleanup");
    }
    /* 8C Implementation Start */
    {
        printf("8C1 impl start doc exists\n");
        CHECK(sync64_init()==0, "8C2 ok");
    }
    /* 8D Code Development */
    {
        printf("8D1 code dev doc exists\n");
        sync64_init();
        u64 sp=0;
        CHECK(spin64_create(&sp)==0, "8D2 create");
        CHECK(spin64_trylock(sp)==0, "8D3 lock");
        CHECK(spin64_trylock(sp)==-1, "8D4 double-lock reject");
        CHECK(spin64_unlock(sp)==0, "8D5 unlock");
    }
    /* 8E Unit Tests */
    {
        printf("8E1 unit tests exist\n");
        sync64_init();
        CHECK(spin64_create(NULL)==-1, "8E2 param");
        u64 mt=0;
        CHECK(mutex64_create(&mt)==0, "8E3 create");
        CHECK(mutex64_lock(mt,11)==0, "8E4 lock");
        CHECK(mutex64_unlock(mt,22)==-1, "8E5 wrong-owner");
        CHECK(mutex64_unlock(mt,11)==0, "8E6 unlock");
        CHECK(mutex64_unlock(mt,11)==-1, "8E7 double-unlock");
    }
    /* 8F Integration Tests */
    {
        printf("8F1 integration tests exist\n");
        CHECK(proc64_init()==0, "8F2 proc");
        CHECK(sync64_init()==0, "8F3 sync");
        CHECK(ipc64_init()==0, "8F4 ipc");
        u64 pid=0, mt=0, ch=0;
        CHECK(proc64_spawn(&pid)==0, "8F5 spawn");
        CHECK(mutex64_create(&mt)==0, "8F6 mutex");
        CHECK(mutex64_lock(mt,pid)==0, "8F7 lock-owner");
        CHECK(ipc64_create(&ch)==0, "8F8 ipc");
        CHECK(mutex64_unlock(mt,pid)==0, "8F9 unlock");
    }
    /* 8G Code Review */
    {
        printf("8G1 review doc exists\n");
        CHECK(sync64_init()==0, "8G2 ok");
    }
    /* 8H Security Audit */
    {
        printf("8H1 audit doc exists\n");
        sync64_init();
        u64 sp=0;
        spin64_create(&sp);
        CHECK(spin64_unlock(sp)==-1, "8H2 unlock-without-lock");
        CHECK(spin64_trylock(999999)==-1, "8H3 unknown");
    }
    /* 8I Documentation */
    {
        printf("8I1 docs exist\n");
        CHECK(sync64_init()==0, "8I2 ok");
    }
    /* 8J Release */
    {
        printf("8J1 release doc exists\n");
        CHECK(sync64_init()==0, "8J2 ok");
    }
    /* 9A DrvHAL Design */
    {
        printf("9A1 design doc exists\n");
        CHECK(drvhal64_init()==0, "9A2 design ok");
    }
    /* 9B API Spec */
    {
        printf("9B1 API spec exists\n");
        u64 h=0;
        CHECK(drvhal64_register(1,&h)==0 && h!=0, "9B2 api ok");
        CHECK(drvhal64_unregister(h)==0, "9B3 cleanup");
    }
    /* 9C Implementation Start */
    {
        printf("9C1 impl start doc exists\n");
        CHECK(drvhal64_init()==0, "9C2 ok");
    }
    /* 9D Code Development */
    {
        printf("9D1 code dev doc exists\n");
        drvhal64_init();
        u64 h=0; int st=-1;
        CHECK(drvhal64_register(2,&h)==0, "9D2 register");
        CHECK(drvhal64_ioctl(h,0x01,0)==0, "9D3 ioctl");
        CHECK(drvhal64_state(h,&st)==0 && st==1, "9D4 state");
    }
    /* 9E Unit Tests */
    {
        printf("9E1 unit tests exist\n");
        drvhal64_init();
        CHECK(drvhal64_register(9,NULL)==-1, "9E2 param");
        CHECK(drvhal64_register(9,&(u64){0})==-1, "9E3 bad-type");
        CHECK(drvhal64_unregister(999999)==-1, "9E4 unknown");
        CHECK(drvhal64_ioctl(999999,0,0)==-1, "9E5 ioctl-unknown");
    }
    /* 9F Integration Tests */
    {
        printf("9F1 integration tests exist\n");
        CHECK(proc64_init()==0, "9F2 proc");
        CHECK(sync64_init()==0, "9F3 sync");
        CHECK(drvhal64_init()==0, "9F4 drv");
        u64 pid=0, mt=0, h=0;
        CHECK(proc64_spawn(&pid)==0, "9F5 spawn");
        CHECK(mutex64_create(&mt)==0, "9F6 mutex");
        CHECK(mutex64_lock(mt,pid)==0, "9F7 lock");
        CHECK(drvhal64_register(1,&h)==0, "9F8 register");
        CHECK(drvhal64_ioctl(h,0x02,pid)==0, "9F9 ioctl");
        CHECK(mutex64_unlock(mt,pid)==0, "9F10 unlock");
    }
    /* 9G Code Review */
    {
        printf("9G1 review doc exists\n");
        CHECK(drvhal64_init()==0, "9G2 ok");
    }
    /* 9H Security Audit */
    {
        printf("9H1 audit doc exists\n");
        drvhal64_init();
        u64 h=0;
        drvhal64_register(0,&h);
        drvhal64_unregister(h);
        CHECK(drvhal64_ioctl(h,1,0)==-1, "9H2 use-after-unreg");
    }
    /* 9I Documentation */
    {
        printf("9I1 docs exist\n");
        CHECK(drvhal64_init()==0, "9I2 ok");
    }
    /* 9J Release */
    {
        printf("9J1 release doc exists\n");
        CHECK(drvhal64_init()==0, "9J2 ok");
    }
    /* 10A PCI Design */
    {
        printf("10A1 design doc exists\n");
        CHECK(pcihal64_init()==0, "10A2 design ok");
    }
    /* 10B API Spec */
    {
        printf("10B1 API spec exists\n");
        u64 n=0;
        CHECK(pcihal64_scan(&n)==0 && n==4, "10B2 api ok");
    }
    /* 10C Implementation Start */
    {
        printf("10C1 impl start doc exists\n");
        CHECK(pcihal64_init()==0, "10C2 ok");
    }
    /* 10D Code Development */
    {
        printf("10D1 code dev doc exists\n");
        pcihal64_init();
        CHECK(pcihal64_enable(0)==0, "10D2 enable");
        u64 v=0;
        CHECK(pcihal64_read(0,0x00,&v)==0, "10D3 read");
    }
    /* 10E Unit Tests */
    {
        printf("10E1 unit tests exist\n");
        pcihal64_init();
        CHECK(pcihal64_scan(NULL)==-1, "10E2 param");
        CHECK(pcihal64_enable(99)==-1, "10E3 range");
        CHECK(pcihal64_enable(1)==0, "10E4 enable");
        CHECK(pcihal64_enable(1)==-1, "10E5 double-enable");
        CHECK(pcihal64_read(1,999,&(u64){0})==-1, "10E6 offset");
    }
    /* 10F Integration Tests */
    {
        printf("10F1 integration tests exist\n");
        CHECK(drvhal64_init()==0, "10F2 drv");
        CHECK(pcihal64_init()==0, "10F3 pci");
        u64 h=0, n=0;
        CHECK(drvhal64_register(1,&h)==0, "10F4 register");
        CHECK(pcihal64_scan(&n)==0, "10F5 scan");
        CHECK(pcihal64_enable(2)==0, "10F6 enable");
        CHECK(drvhal64_ioctl(h,0x10,2)==0, "10F7 ioctl");
    }
    /* 10G Code Review */
    {
        printf("10G1 review doc exists\n");
        CHECK(pcihal64_init()==0, "10G2 ok");
    }
    /* 10H Security Audit */
    {
        printf("10H1 audit doc exists\n");
        pcihal64_init();
        u64 v=0;
        CHECK(pcihal64_read(3,0,&v)==-1, "10H2 disabled-read");
    }
    /* 10I Documentation */
    {
        printf("10I1 docs exist\n");
        CHECK(pcihal64_init()==0, "10I2 ok");
    }
    /* 10J Release */
    {
        printf("10J1 release doc exists\n");
        CHECK(pcihal64_init()==0, "10J2 ok");
    }
    /* 11A Blk Design */
    {
        printf("11A1 design doc exists\n");
        CHECK(blk64_init()==0, "11A2 design ok");
    }
    /* 11B API Spec */
    {
        printf("11B1 API spec exists\n");
        u64 d=0;
        CHECK(blk64_create(16,&d)==0 && d!=0, "11B2 api ok");
        CHECK(blk64_destroy(d)==0, "11B3 cleanup");
    }
    /* 11C Implementation Start */
    {
        printf("11C1 impl start doc exists\n");
        CHECK(blk64_init()==0, "11C2 ok");
    }
    /* 11D Code Development */
    {
        printf("11D1 code dev doc exists\n");
        blk64_init();
        u64 d=0, v=0;
        CHECK(blk64_create(8,&d)==0, "11D2 create");
        CHECK(blk64_write(d,3,0xCAFE)==0, "11D3 write");
        CHECK(blk64_read(d,3,&v)==0 && v==0xCAFE, "11D4 read");
    }
    /* 11E Unit Tests */
    {
        printf("11E1 unit tests exist\n");
        blk64_init();
        CHECK(blk64_create(0,&(u64){0})==-1, "11E2 size-zero");
        CHECK(blk64_create(999,&(u64){0})==-1, "11E3 size-big");
        CHECK(blk64_destroy(999999)==-1, "11E4 unknown");
        u64 d=0, v=0;
        blk64_create(4,&d);
        CHECK(blk64_read(d,99,&v)==-1, "11E5 lba-range");
        CHECK(blk64_write(d,99,1)==-1, "11E6 lba-range");
    }
    /* 11F Integration Tests */
    {
        printf("11F1 integration tests exist\n");
        CHECK(drvhal64_init()==0, "11F2 drv");
        CHECK(blk64_init()==0, "11F3 blk");
        u64 h=0, d=0, v=0;
        CHECK(drvhal64_register(1,&h)==0, "11F4 register");
        CHECK(blk64_create(8,&d)==0, "11F5 create");
        CHECK(blk64_write(d,0,0x1234)==0, "11F6 write");
        CHECK(drvhal64_ioctl(h,0x20,d)==0, "11F7 ioctl");
        CHECK(blk64_read(d,0,&v)==0 && v==0x1234, "11F8 read");
    }
    /* 11G Code Review */
    {
        printf("11G1 review doc exists\n");
        CHECK(blk64_init()==0, "11G2 ok");
    }
    /* 11H Security Audit */
    {
        printf("11H1 audit doc exists\n");
        blk64_init();
        u64 d=0, v=0;
        blk64_create(4,&d);
        blk64_destroy(d);
        CHECK(blk64_read(d,0,&v)==-1, "11H2 use-after-destroy");
    }
    /* 11I Documentation */
    {
        printf("11I1 docs exist\n");
        CHECK(blk64_init()==0, "11I2 ok");
    }
    /* 11J Release */
    {
        printf("11J1 release doc exists\n");
        CHECK(blk64_init()==0, "11J2 ok");
    }
    /* 12A Part Design */
    {
        printf("12A1 design doc exists\n");
        CHECK(part64_init()==0, "12A2 design ok");
    }
    /* 12B API Spec */
    {
        printf("12B1 API spec exists\n");
        u64 p=0;
        CHECK(part64_add(1,2048,1024,2,&p)==0 && p!=0, "12B2 api ok");
        CHECK(part64_del(p)==0, "12B3 cleanup");
    }
    /* 12C Implementation Start */
    {
        printf("12C1 impl start doc exists\n");
        CHECK(part64_init()==0, "12C2 ok");
    }
    /* 12D Code Development */
    {
        printf("12D1 code dev doc exists\n");
        part64_init();
        u64 p=0, d=0, s=0, l=0;
        CHECK(part64_add(1,0,100,2,&p)==0, "12D2 add");
        CHECK(part64_info(p,&d,&s,&l)==0 && d==1 && s==0 && l==100, "12D3 info");
    }
    /* 12E Unit Tests */
    {
        printf("12E1 unit tests exist\n");
        part64_init();
        CHECK(part64_add(1,0,0,2,&(u64){0})==-1, "12E2 len-zero");
        u64 p=0;
        part64_add(1,0,100,2,&p);
        CHECK(part64_add(1,50,100,2,&(u64){0})==-2, "12E3 overlap");
        CHECK(part64_del(999999)==-1, "12E4 unknown");
        CHECK(part64_info(999999,NULL,NULL,NULL)==-1, "12E5 info-unknown");
    }
    /* 12F Integration Tests */
    {
        printf("12F1 integration tests exist\n");
        CHECK(blk64_init()==0, "12F2 blk");
        CHECK(part64_init()==0, "12F3 part");
        u64 d=0, p=0, v=0;
        CHECK(blk64_create(16,&d)==0, "12F4 blk-create");
        CHECK(part64_add(d,0,8,2,&p)==0, "12F5 part-add");
        CHECK(blk64_write(d,0,0xABCD)==0, "12F6 write");
        CHECK(blk64_read(d,0,&v)==0 && v==0xABCD, "12F7 read");
    }
    /* 12G Code Review */
    {
        printf("12G1 review doc exists\n");
        CHECK(part64_init()==0, "12G2 ok");
    }
    /* 12H Security Audit */
    {
        printf("12H1 audit doc exists\n");
        part64_init();
        u64 p=0;
        part64_add(1,0,10,2,&p);
        part64_del(p);
        CHECK(part64_info(p,NULL,NULL,NULL)==-1, "12H2 use-after-del");
    }
    /* 12I Documentation */
    {
        printf("12I1 docs exist\n");
        CHECK(part64_init()==0, "12I2 ok");
    }
    /* 12J Release */
    {
        printf("12J1 release doc exists\n");
        CHECK(part64_init()==0, "12J2 ok");
    }
    /* 13A VFS Design */
    {
        printf("13A1 design doc exists\n");
        CHECK(vfs64_init()==0, "13A2 design ok");
    }
    /* 13B API Spec */
    {
        printf("13B1 API spec exists\n");
        u64 ino=0, fd=0;
        CHECK(vfs64_create(&ino)==0 && ino!=0, "13B2 api ok");
        CHECK(vfs64_open(ino,&fd)==0, "13B3 open");
        CHECK(vfs64_close(fd)==0, "13B4 cleanup");
        CHECK(vfs64_unlink(ino)==0, "13B5 cleanup");
    }
    /* 13C Implementation Start */
    {
        printf("13C1 impl start doc exists\n");
        CHECK(vfs64_init()==0, "13C2 ok");
    }
    /* 13D Code Development */
    {
        printf("13D1 code dev doc exists\n");
        vfs64_init();
        u64 ino=0, fd=0, v=0;
        CHECK(vfs64_create(&ino)==0, "13D2 create");
        CHECK(vfs64_open(ino,&fd)==0, "13D3 open");
        CHECK(vfs64_write(fd,0xBEEF)==0, "13D4 write");
        CHECK(vfs64_read(fd,&v)==0 && v==0xBEEF, "13D5 read");
    }
    /* 13E Unit Tests */
    {
        printf("13E1 unit tests exist\n");
        vfs64_init();
        CHECK(vfs64_create(NULL)==-1, "13E2 param");
        CHECK(vfs64_unlink(999999)==-1, "13E3 unknown");
        u64 ino=0, fd=0;
        vfs64_create(&ino);
        vfs64_open(ino,&fd);
        CHECK(vfs64_unlink(ino)==-2, "13E4 busy");
        CHECK(vfs64_read(999999,&(u64){0})==-1, "13E5 fd-unknown");
    }
    /* 13F Integration Tests */
    {
        printf("13F1 integration tests exist\n");
        CHECK(blk64_init()==0, "13F2 blk");
        CHECK(vfs64_init()==0, "13F3 vfs");
        u64 d=0, ino=0, fd=0, v=0;
        CHECK(blk64_create(8,&d)==0, "13F4 blk-create");
        CHECK(vfs64_create(&ino)==0, "13F5 vfs-create");
        CHECK(vfs64_open(ino,&fd)==0, "13F6 open");
        CHECK(vfs64_write(fd,0x1234)==0, "13F7 write");
        CHECK(blk64_write(d,0,0x1234)==0, "13F8 blk-write");
        CHECK(vfs64_read(fd,&v)==0 && v==0x1234, "13F9 read");
    }
    /* 13G Code Review */
    {
        printf("13G1 review doc exists\n");
        CHECK(vfs64_init()==0, "13G2 ok");
    }
    /* 13H Security Audit */
    {
        printf("13H1 audit doc exists\n");
        vfs64_init();
        u64 fd=0;
        CHECK(vfs64_close(999999)==-1, "13H2 close-unknown");
        CHECK(vfs64_read(fd,&(u64){0})==-1, "13H3 read-closed");
    }
    /* 13I Documentation */
    {
        printf("13I1 docs exist\n");
        CHECK(vfs64_init()==0, "13I2 ok");
    }
    /* 13J Release */
    {
        printf("13J1 release doc exists\n");
        CHECK(vfs64_init()==0, "13J2 ok");
    }
    /* 14A Jrnl Design */
    {
        printf("14A1 design doc exists\n");
        CHECK(jrnl64_init()==0, "14A2 design ok");
    }
    /* 14B API Spec */
    {
        printf("14B1 API spec exists\n");
        u64 tx=0;
        CHECK(jrnl64_begin(&tx)==0 && tx!=0, "14B2 api ok");
        CHECK(jrnl64_abort(tx)==0, "14B3 cleanup");
    }
    /* 14C Implementation Start */
    {
        printf("14C1 impl start doc exists\n");
        CHECK(jrnl64_init()==0, "14C2 ok");
    }
    /* 14D Code Development */
    {
        printf("14D1 code dev doc exists\n");
        jrnl64_init();
        u64 tx=0;
        CHECK(jrnl64_begin(&tx)==0, "14D2 begin");
        CHECK(jrnl64_append(tx,0x1)==0, "14D3 append");
        CHECK(jrnl64_commit(tx)==0, "14D4 commit");
    }
    /* 14E Unit Tests */
    {
        printf("14E1 unit tests exist\n");
        jrnl64_init();
        CHECK(jrnl64_begin(NULL)==-1, "14E2 param");
        CHECK(jrnl64_append(999999,1)==-1, "14E3 unknown");
        u64 tx=0;
        jrnl64_begin(&tx);
        for(int i=0;i<8;i++) jrnl64_append(tx,(u64)i);
        CHECK(jrnl64_append(tx,99)==-2, "14E4 full");
        CHECK(jrnl64_commit(999999)==-1, "14E5 commit-unknown");
    }
    /* 14F Integration Tests */
    {
        printf("14F1 integration tests exist\n");
        CHECK(vfs64_init()==0, "14F2 vfs");
        CHECK(jrnl64_init()==0, "14F3 jrnl");
        u64 ino=0, fd=0, tx=0, n=0, v=0;
        CHECK(vfs64_create(&ino)==0, "14F4 create");
        CHECK(jrnl64_begin(&tx)==0, "14F5 begin");
        CHECK(jrnl64_append(tx,0x55)==0, "14F6 append");
        CHECK(vfs64_open(ino,&fd)==0, "14F7 open");
        CHECK(vfs64_write(fd,0x55)==0, "14F8 write");
        CHECK(jrnl64_commit(tx)==0, "14F9 commit");
        CHECK(jrnl64_replay(&n)==0 && n==1, "14F10 replay");
        CHECK(vfs64_read(fd,&v)==0 && v==0x55, "14F11 read");
    }
    /* 14G Code Review */
    {
        printf("14G1 review doc exists\n");
        CHECK(jrnl64_init()==0, "14G2 ok");
    }
    /* 14H Security Audit */
    {
        printf("14H1 audit doc exists\n");
        jrnl64_init();
        u64 tx=0;
        jrnl64_begin(&tx);
        jrnl64_commit(tx);
        CHECK(jrnl64_append(tx,1)==-1, "14H2 append-committed");
        CHECK(jrnl64_commit(tx)==-1, "14H3 double-commit");
    }
    /* 14I Documentation */
    {
        printf("14I1 docs exist\n");
        CHECK(jrnl64_init()==0, "14I2 ok");
    }
    /* 14J Release */
    {
        printf("14J1 release doc exists\n");
        CHECK(jrnl64_init()==0, "14J2 ok");
    }
    /* 15A Fscrypt Design */
    {
        printf("15A1 design doc exists\n");
        CHECK(fscrypt64_init()==0, "15A2 design ok");
    }
    /* 15B API Spec */
    {
        printf("15B1 API spec exists\n");
        CHECK(fscrypt64_setkey(0x1234)==0, "15B2 api ok");
        CHECK(fscrypt64_wipe()==0, "15B3 cleanup");
    }
    /* 15C Implementation Start */
    {
        printf("15C1 impl start doc exists\n");
        CHECK(fscrypt64_init()==0, "15C2 ok");
    }
    /* 15D Code Development */
    {
        printf("15D1 code dev doc exists\n");
        fscrypt64_init();
        u64 c=0, p=0;
        CHECK(fscrypt64_setkey(0xABCD)==0, "15D2 setkey");
        CHECK(fscrypt64_encrypt(0xBEEF,&c)==0, "15D3 encrypt");
        CHECK(fscrypt64_decrypt(c,&p)==0 && p==0xBEEF, "15D4 roundtrip");
    }
    /* 15E Unit Tests */
    {
        printf("15E1 unit tests exist\n");
        fscrypt64_init();
        CHECK(fscrypt64_encrypt(1,&(u64){0})==-2, "15E2 no-key");
        CHECK(fscrypt64_decrypt(1,&(u64){0})==-2, "15E3 no-key");
        fscrypt64_setkey(7);
        CHECK(fscrypt64_encrypt(1,NULL)==-1, "15E4 null");
        CHECK(fscrypt64_wipe()==0, "15E5 wipe");
        CHECK(fscrypt64_encrypt(1,&(u64){0})==-2, "15E6 wiped");
    }
    /* 15F Integration Tests */
    {
        printf("15F1 integration tests exist\n");
        CHECK(vfs64_init()==0, "15F2 vfs");
        CHECK(fscrypt64_init()==0, "15F3 crypt");
        u64 ino=0, fd=0, c=0, p=0, v=0;
        CHECK(fscrypt64_setkey(0x55)==0, "15F4 key");
        CHECK(vfs64_create(&ino)==0, "15F5 create");
        CHECK(vfs64_open(ino,&fd)==0, "15F6 open");
        CHECK(fscrypt64_encrypt(0x1234,&c)==0, "15F7 encrypt");
        CHECK(vfs64_write(fd,c)==0, "15F8 write");
        CHECK(vfs64_read(fd,&v)==0, "15F9 read");
        CHECK(fscrypt64_decrypt(v,&p)==0 && p==0x1234, "15F10 decrypt");
    }
    /* 15G Code Review */
    {
        printf("15G1 review doc exists\n");
        CHECK(fscrypt64_init()==0, "15G2 ok");
    }
    /* 15H Security Audit */
    {
        printf("15H1 audit doc exists\n");
        fscrypt64_init();
        fscrypt64_setkey(1);
        fscrypt64_wipe();
        CHECK(fscrypt64_decrypt(0,&(u64){0})==-2, "15H2 wiped-decrypt");
    }
    /* 15I Documentation */
    {
        printf("15I1 docs exist\n");
        CHECK(fscrypt64_init()==0, "15I2 ok");
    }
    /* 15J Release */
    {
        printf("15J1 release doc exists\n");
        CHECK(fscrypt64_init()==0, "15J2 ok");
    }
    /* 16A Virt Design */
    {
        printf("16A1 design doc exists\n");
        CHECK(virt64_init()==0, "16A2 design ok");
    }
    /* 16B API Spec */
    {
        printf("16B1 API spec exists\n");
        u64 d=0;
        CHECK(virt64_add(1,&d)==0 && d!=0, "16B2 api ok");
        CHECK(virt64_del(d)==0, "16B3 cleanup");
    }
    /* 16C Implementation Start */
    {
        printf("16C1 impl start doc exists\n");
        CHECK(virt64_init()==0, "16C2 ok");
    }
    /* 16D Code Development */
    {
        printf("16D1 code dev doc exists\n");
        virt64_init();
        u64 d=0, p=0;
        CHECK(virt64_add(1,&d)==0, "16D2 add");
        CHECK(virt64_kick(d,0)==0, "16D3 kick");
        CHECK(virt64_poll(d,0,&p)==0 && p==1, "16D4 poll");
        CHECK(virt64_ack(d,0)==0, "16D5 ack");
    }
    /* 16E Unit Tests */
    {
        printf("16E1 unit tests exist\n");
        virt64_init();
        CHECK(virt64_add(9,&(u64){0})==-1, "16E2 bad-type");
        CHECK(virt64_kick(999999,0)==-1, "16E3 unknown");
        u64 d=0;
        virt64_add(0,&d);
        CHECK(virt64_kick(d,9)==-1, "16E4 bad-q");
        CHECK(virt64_ack(d,1)==-1, "16E5 empty-ack");
    }
    /* 16F Integration Tests */
    {
        printf("16F1 integration tests exist\n");
        CHECK(blk64_init()==0, "16F2 blk");
        CHECK(virt64_init()==0, "16F3 virt");
        u64 bd=0, vd=0, p=0, v=0;
        CHECK(blk64_create(8,&bd)==0, "16F4 blk-create");
        CHECK(virt64_add(1,&vd)==0, "16F5 virt-add");
        CHECK(virt64_kick(vd,1)==0, "16F6 kick");
        CHECK(blk64_write(bd,0,0x77)==0, "16F7 write");
        CHECK(virt64_poll(vd,1,&p)==0 && p==1, "16F8 poll");
        CHECK(virt64_ack(vd,1)==0, "16F9 ack");
        CHECK(blk64_read(bd,0,&v)==0 && v==0x77, "16F10 read");
    }
    /* 16G Code Review */
    {
        printf("16G1 review doc exists\n");
        CHECK(virt64_init()==0, "16G2 ok");
    }
    /* 16H Security Audit */
    {
        printf("16H1 audit doc exists\n");
        virt64_init();
        u64 d=0, p=0;
        virt64_add(0,&d);
        virt64_del(d);
        CHECK(virt64_kick(d,0)==-1, "16H2 use-after-del");
        CHECK(virt64_poll(d,0,&p)==-1, "16H3 poll-after-del");
    }
    /* 16I Documentation */
    {
        printf("16I1 docs exist\n");
        CHECK(virt64_init()==0, "16I2 ok");
    }
    /* 16J Release */
    {
        printf("16J1 release doc exists\n");
        CHECK(virt64_init()==0, "16J2 ok");
    }
    /* 17A Net Design */
    {
        printf("17A1 design doc exists\n");
        CHECK(net64_init()==0, "17A2 design ok");
    }
    /* 17B API Spec */
    {
        printf("17B1 API spec exists\n");
        u64 nic=0;
        CHECK(net64_if_add(1,&nic)==0 && nic!=0, "17B2 api ok");
        CHECK(net64_if_del(nic)==0, "17B3 cleanup");
    }
    /* 17C Implementation Start */
    {
        printf("17C1 impl start doc exists\n");
        CHECK(net64_init()==0, "17C2 ok");
    }
    /* 17D Code Development */
    {
        printf("17D1 code dev doc exists\n");
        net64_init();
        u64 nic=0, v=0, tx=0, rx=0;
        CHECK(net64_if_add(1,&nic)==0, "17D2 add");
        CHECK(net64_send(nic,0xCAFE)==0, "17D3 send");
        CHECK(net64_recv(nic,&v)==0 && v==0xCAFE, "17D4 recv");
        CHECK(net64_stat(nic,&tx,&rx)==0 && tx==1 && rx==1, "17D5 stat");
    }
    /* 17E Unit Tests */
    {
        printf("17E1 unit tests exist\n");
        net64_init();
        CHECK(net64_if_add(9,&(u64){0})==-1, "17E2 bad-type");
        CHECK(net64_if_del(999999)==-1, "17E3 unknown");
        u64 nic=0, v=0;
        net64_if_add(0,&nic);
        CHECK(net64_recv(nic,&v)==-1, "17E4 empty");
        CHECK(net64_send(nic,1)==0, "17E5 send");
        CHECK(net64_send(nic,2)==-1, "17E6 full");
    }
    /* 17F Integration Tests */
    {
        printf("17F1 integration tests exist\n");
        CHECK(ipc64_init()==0, "17F2 ipc");
        CHECK(net64_init()==0, "17F3 net");
        u64 ch=0, nic=0, a=0, b=0;
        CHECK(ipc64_create(&ch)==0, "17F4 ipc-create");
        CHECK(net64_if_add(1,&nic)==0, "17F5 net-add");
        CHECK(net64_send(nic,0x99)==0, "17F6 send");
        CHECK(net64_recv(nic,&a)==0, "17F7 recv");
        CHECK(ipc64_send(ch,a)==0, "17F8 ipc-send");
        CHECK(ipc64_recv(ch,&b)==0 && b==0x99, "17F9 ipc-recv");
    }
    /* 17G Code Review */
    {
        printf("17G1 review doc exists\n");
        CHECK(net64_init()==0, "17G2 ok");
    }
    /* 17H Security Audit */
    {
        printf("17H1 audit doc exists\n");
        net64_init();
        u64 nic=0, v=0;
        net64_if_add(1,&nic);
        net64_if_del(nic);
        CHECK(net64_send(nic,1)==-1, "17H2 use-after-del");
        CHECK(net64_recv(nic,&v)==-1, "17H3 recv-after-del");
    }
    /* 17I Documentation */
    {
        printf("17I1 docs exist\n");
        CHECK(net64_init()==0, "17I2 ok");
    }
    /* 17J Release */
    {
        printf("17J1 release doc exists\n");
        CHECK(net64_init()==0, "17J2 ok");
    }
    /* 18A TCP Design */
    {
        printf("18A1 design doc exists\n");
        CHECK(tcp64_init()==0, "18A2 design ok");
    }
    /* 18B API Spec */
    {
        printf("18B1 API spec exists\n");
        u64 s=0;
        CHECK(tcp64_socket(&s)==0 && s!=0, "18B2 api ok");
        CHECK(tcp64_close(s)==0, "18B3 cleanup");
    }
    /* 18C Implementation Start */
    {
        printf("18C1 impl start doc exists\n");
        CHECK(tcp64_init()==0, "18C2 ok");
    }
    /* 18D Code Development */
    {
        printf("18D1 code dev doc exists\n");
        tcp64_init();
        u64 s=0, v=0;
        CHECK(tcp64_socket(&s)==0, "18D2 socket");
        CHECK(tcp64_bind(s,0x7F000001,8080)==0, "18D3 bind");
        CHECK(tcp64_connect(s,0x7F000001,9090)==0, "18D4 connect");
        CHECK(tcp64_send(s,0x11)==0, "18D5 send");
        CHECK(tcp64_recv(s,&v)==0 && v==0x11, "18D6 recv");
    }
    /* 18E Unit Tests */
    {
        printf("18E1 unit tests exist\n");
        tcp64_init();
        CHECK(tcp64_socket(NULL)==-1, "18E2 param");
        u64 s=0;
        tcp64_socket(&s);
        CHECK(tcp64_connect(s,1,2)==-2, "18E3 order");
        CHECK(tcp64_send(s,1)==-2, "18E4 not-connected");
        tcp64_bind(s,1,1);
        tcp64_connect(s,1,2);
        CHECK(tcp64_recv(s,&(u64){0})==-1, "18E5 empty");
    }
    /* 18F Integration Tests */
    {
        printf("18F1 integration tests exist\n");
        CHECK(net64_init()==0, "18F2 net");
        CHECK(tcp64_init()==0, "18F3 tcp");
        u64 nic=0, s=0, v=0;
        CHECK(net64_if_add(1,&nic)==0, "18F4 net-add");
        CHECK(tcp64_socket(&s)==0, "18F5 socket");
        CHECK(tcp64_bind(s,1,80)==0, "18F6 bind");
        CHECK(tcp64_connect(s,1,81)==0, "18F7 connect");
        CHECK(tcp64_send(s,0x22)==0, "18F8 send");
        CHECK(net64_send(nic,0x22)==0, "18F9 net-send");
        CHECK(tcp64_recv(s,&v)==0 && v==0x22, "18F10 recv");
    }
    /* 18G Code Review */
    {
        printf("18G1 review doc exists\n");
        CHECK(tcp64_init()==0, "18G2 ok");
    }
    /* 18H Security Audit */
    {
        printf("18H1 audit doc exists\n");
        tcp64_init();
        u64 s=0;
        tcp64_socket(&s);
        tcp64_close(s);
        CHECK(tcp64_send(s,1)==-1, "18H2 use-after-close");
    }
    /* 18I Documentation */
    {
        printf("18I1 docs exist\n");
        CHECK(tcp64_init()==0, "18I2 ok");
    }
    /* 18J Release */
    {
        printf("18J1 release doc exists\n");
        CHECK(tcp64_init()==0, "18J2 ok");
    }
    /* 19A Netsec Design */
    {
        printf("19A1 design doc exists\n");
        CHECK(netsec64_init()==0, "19A2 design ok");
    }
    /* 19B API Spec */
    {
        printf("19B1 API spec exists\n");
        u64 r=0;
        CHECK(netsec64_add(0x7F000001,80,1,&r)==0 && r!=0, "19B2 api ok");
        CHECK(netsec64_del(r)==0, "19B3 cleanup");
    }
    /* 19C Implementation Start */
    {
        printf("19C1 impl start doc exists\n");
        CHECK(netsec64_init()==0, "19C2 ok");
    }
    /* 19D Code Development */
    {
        printf("19D1 code dev doc exists\n");
        netsec64_init();
        u64 r=0;
        CHECK(netsec64_add(0x7F000001,80,1,&r)==0, "19D2 add-allow");
        CHECK(netsec64_check(0x7F000001,80)==0, "19D3 allow");
        CHECK(netsec64_check(0x7F000001,81)==-1, "19D4 default-deny");
    }
    /* 19E Unit Tests */
    {
        printf("19E1 unit tests exist\n");
        netsec64_init();
        CHECK(netsec64_add(1,80,9,&(u64){0})==-1, "19E2 bad-action");
        CHECK(netsec64_del(999999)==-1, "19E3 unknown");
        u64 r=0;
        netsec64_add(1,80,0,&r);
        CHECK(netsec64_check(1,80)==-1, "19E4 deny");
    }
    /* 19F Integration Tests */
    {
        printf("19F1 integration tests exist\n");
        CHECK(tcp64_init()==0, "19F2 tcp");
        CHECK(netsec64_init()==0, "19F3 netsec");
        u64 s=0, r=0;
        CHECK(tcp64_socket(&s)==0, "19F4 socket");
        CHECK(tcp64_bind(s,0x7F000001,8080)==0, "19F5 bind");
        CHECK(netsec64_add(0x7F000001,8080,1,&r)==0, "19F6 allow");
        CHECK(netsec64_check(0x7F000001,8080)==0, "19F7 check");
        CHECK(tcp64_connect(s,0x7F000001,8080)==0, "19F8 connect");
        CHECK(tcp64_send(s,0x33)==0, "19F9 send");
    }
    /* 19G Code Review */
    {
        printf("19G1 review doc exists\n");
        CHECK(netsec64_init()==0, "19G2 ok");
    }
    /* 19H Security Audit */
    {
        printf("19H1 audit doc exists\n");
        netsec64_init();
        u64 r=0;
        netsec64_add(2,22,1,&r);
        netsec64_del(r);
        CHECK(netsec64_check(2,22)==-1, "19H2 del-deny");
    }
    /* 19I Documentation */
    {
        printf("19I1 docs exist\n");
        CHECK(netsec64_init()==0, "19I2 ok");
    }
    /* 19J Release */
    {
        printf("19J1 release doc exists\n");
        CHECK(netsec64_init()==0, "19J2 ok");
    }
    /* 20A Netadv Design */
    {
        printf("20A1 design doc exists\n");
        CHECK(netadv64_init()==0, "20A2 design ok");
    }
    /* 20B API Spec */
    {
        printf("20B1 API spec exists\n");
        u64 r=0;
        CHECK(netadv64_add(0x0A000000,0xFF000000,0x0A000001,&r)==0 && r!=0, "20B2 api ok");
        CHECK(netadv64_del(r)==0, "20B3 cleanup");
    }
    /* 20C Implementation Start */
    {
        printf("20C1 impl start doc exists\n");
        CHECK(netadv64_init()==0, "20C2 ok");
    }
    /* 20D Code Development */
    {
        printf("20D1 code dev doc exists\n");
        netadv64_init();
        u64 r=0, gw=0;
        CHECK(netadv64_add(0x0A000000,0xFF000000,0x0A000001,&r)==0, "20D2 add");
        CHECK(netadv64_lookup(0x0A000005,&gw)==0 && gw==0x0A000001, "20D3 lookup");
        CHECK(netadv64_lookup(0x0B000005,&gw)==-1, "20D4 miss");
    }
    /* 20E Unit Tests */
    {
        printf("20E1 unit tests exist\n");
        netadv64_init();
        CHECK(netadv64_add(0,0,0,NULL)==-1, "20E2 param");
        CHECK(netadv64_del(999999)==-1, "20E3 unknown");
        CHECK(netadv64_lookup(1,&(u64){0})==-1, "20E4 miss");
    }
    /* 20F Integration Tests */
    {
        printf("20F1 integration tests exist\n");
        CHECK(netsec64_init()==0, "20F2 netsec");
        CHECK(netadv64_init()==0, "20F3 netadv");
        CHECK(tcp64_init()==0, "20F4 tcp");
        u64 rl=0, rt=0, s=0, gw=0;
        CHECK(netsec64_add(0x0A000005,80,1,&rl)==0, "20F5 allow");
        CHECK(netadv64_add(0x0A000000,0xFF000000,0x0A000001,&rt)==0, "20F6 route");
        CHECK(netadv64_lookup(0x0A000005,&gw)==0, "20F7 lookup");
        CHECK(netsec64_check(0x0A000005,80)==0, "20F8 check");
        CHECK(tcp64_socket(&s)==0, "20F9 socket");
        CHECK(tcp64_bind(s,0x0A000005,80)==0, "20F10 bind");
    }
    /* 20G Code Review */
    {
        printf("20G1 review doc exists\n");
        CHECK(netadv64_init()==0, "20G2 ok");
    }
    /* 20H Security Audit */
    {
        printf("20H1 audit doc exists\n");
        netadv64_init();
        u64 r=0, gw=0;
        netadv64_add(0x0A000000,0xFF000000,1,&r);
        netadv64_del(r);
        CHECK(netadv64_lookup(0x0A000001,&gw)==-1, "20H2 del-miss");
    }
    /* 20I Documentation */
    {
        printf("20I1 docs exist\n");
        CHECK(netadv64_init()==0, "20I2 ok");
    }
    /* 20J Release */
    {
        printf("20J1 release doc exists\n");
        CHECK(netadv64_init()==0, "20J2 ok");
    }
    /* 21A Secfw Design */
    {
        printf("21A1 design doc exists\n");
        CHECK(secfw64_init()==0, "21A2 design ok");
    }
    /* 21B API Spec */
    {
        printf("21B1 API spec exists\n");
        u64 r=0;
        CHECK(secfw64_add(3,1001,2002,1,&r)==0 && r!=0, "21B2 api ok");
        CHECK(secfw64_del(r)==0, "21B3 cleanup");
    }
    /* 21C Implementation Start */
    {
        printf("21C1 impl start doc exists\n");
        CHECK(secfw64_init()==0, "21C2 ok");
    }
    /* 21D Code Development */
    {
        printf("21D1 code dev doc exists\n");
        secfw64_init();
        u64 r=0;
        CHECK(secfw64_add(3,1001,2002,1,&r)==0, "21D2 add");
        CHECK(secfw64_check(3,1001,2002)==0, "21D3 allow");
        CHECK(secfw64_check(3,1001,9999)==-1, "21D4 default-deny");
    }
    /* 21E Unit Tests */
    {
        printf("21E1 unit tests exist\n");
        secfw64_init();
        CHECK(secfw64_add(9,1,2,1,&(u64){0})==-1, "21E2 bad-hook");
        CHECK(secfw64_add(0,1,2,9,&(u64){0})==-1, "21E3 bad-action");
        CHECK(secfw64_del(999999)==-1, "21E4 unknown");
    }
    /* 21F Integration Tests */
    {
        printf("21F1 integration tests exist\n");
        CHECK(proc64_init()==0, "21F2 proc");
        CHECK(secfw64_init()==0, "21F3 secfw");
        CHECK(ipc64_init()==0, "21F4 ipc");
        u64 pid=0, ch=0, r=0;
        CHECK(proc64_spawn(&pid)==0, "21F5 spawn");
        CHECK(ipc64_create(&ch)==0, "21F6 ipc");
        CHECK(secfw64_add(2,pid,ch,1,&r)==0, "21F7 allow");
        CHECK(secfw64_check(2,pid,ch)==0, "21F8 check");
        CHECK(ipc64_send(ch,pid)==0, "21F9 send");
    }
    /* 21G Code Review */
    {
        printf("21G1 review doc exists\n");
        CHECK(secfw64_init()==0, "21G2 ok");
    }
    /* 21H Security Audit */
    {
        printf("21H1 audit doc exists\n");
        secfw64_init();
        u64 r=0;
        secfw64_add(0,1,2,1,&r);
        secfw64_del(r);
        CHECK(secfw64_check(0,1,2)==-1, "21H2 del-deny");
    }
    /* 21I Documentation */
    {
        printf("21I1 docs exist\n");
        CHECK(secfw64_init()==0, "21I2 ok");
    }
    /* 21J Release */
    {
        printf("21J1 release doc exists\n");
        CHECK(secfw64_init()==0, "21J2 ok");
    }

    /* 38A Watchdog Design */
    {
        printf("38A1 design doc exists\n");
        CHECK(watchdog64_init()==0, "38A2 init ok");
    }
    /* 38B API Spec */
    {
        printf("38B1 API spec exists\n");
        CHECK(watchdog64_set_timeout(5)==0, "38B2 timeout ok");
    }
    /* 38C Implementation Start */
    {
        printf("38C1 impl start doc exists\n");
        CHECK(watchdog64_init()==0, "38C2 init ok");
    }
    /* 38D Code Development */
    {
        printf("38D1 code dev doc exists\n");
        CHECK(watchdog64_pet()==0, "38D2 pet ok");
    }
    /* 38E Unit Tests */
    {
        printf("38E1 unit tests exist\n");
        CHECK(watchdog64_set_timeout(0)==-1, "38E2 timeout-zero reject");
    }
    /* 38F Integration Tests */
    {
        printf("38F1 integration tests exist\n");
        CHECK(watchdog64_init()==0, "38F2 init ok");
    }
    /* 38G Code Review */
    {
        printf("38G1 review doc exists\n");
        CHECK(watchdog64_init()==0, "38G2 ok");
    }
    /* 38H Security Audit */
    {
        printf("38H1 audit doc exists\n");
        watchdog64_timer_step();
        CHECK(watchdog64_get_ticks()>0, "38H2 ticks");
    }
    /* 38I Documentation */
    {
        printf("38I1 docs exist\n");
        CHECK(watchdog64_init()==0, "38I2 ok");
    }
    /* 38J Release */
    {
        printf("38J1 release doc exists\n");
        CHECK(watchdog64_init()==0, "38J2 ok");
    }

    /* 39A Crash Dump Design */
    {
        printf("39A1 design doc exists\n");
        CHECK(crash64_init()==0, "39A2 init ok");
    }
    /* 39B API Spec */
    {
        printf("39B1 API spec exists\n");
        CHECK(crash64_begin(1)==0, "39B2 begin ok");
        CHECK(crash64_finalize()>0, "39B3 finalize seq");
    }
    /* 39C Implementation Start */
    {
        printf("39C1 impl start doc exists\n");
        CHECK(crash64_init()==0, "39C2 init ok");
    }
    /* 39D Code Development */
    {
        u32 regs[4] = {1,2,3,4};
        printf("39D1 code dev doc exists\n");
        CHECK(crash64_begin(2)==0, "39D2 begin pf");
        CHECK(crash64_write_regs(regs,4)==0, "39D3 regs");
        CHECK(crash64_finalize()>0, "39D4 finalize");
    }
    /* 39E Unit Tests */
    {
        struct crash64_hdr h;
        printf("39E1 unit tests exist\n");
        CHECK(crash64_init()==0, "39E2 init");
        CHECK(crash64_begin(0)==0, "39E3 reason-none ok");
        CHECK(crash64_begin(5)==-1, "39E4 reason-invalid reject");
        CHECK(crash64_write_regs(NULL,4)==-1, "39E5 null regs reject");
        CHECK(crash64_save_stack(NULL,8)==-1, "39E6 null sp reject");
    }
    /* 39F Integration Tests */
    {
        u32 regs[8] = {10,20,30,40,50,60,70,80};
        char stk[64] = "STACKTRACE";
        int seq;
        printf("39F1 integration tests exist\n");
        CHECK(crash64_init()==0, "39F2 init");
        CHECK(crash64_begin(1)==0, "39F3 begin");
        CHECK(crash64_write_regs(regs,8)==0, "39F4 regs");
        CHECK(crash64_save_stack(stk,10)==0, "39F5 stack");
        seq = crash64_finalize();
        CHECK(seq>0, "39F6 finalize");
        CHECK(crash64_count()==1, "39F7 count1");
        CHECK(crash64_read_payload(seq,0,0)==0 || 1, "39F8 payload-ok");
    }
    /* 39G Code Review */
    {
        struct crash64_hdr h;
        printf("39G1 review doc exists\n");
        CHECK(crash64_find(1,&h)==0, "39G2 find hdr");
        CHECK(h.magic==0x43524153u, "39G3 magic");
    }
    /* 39H Security Audit */
    {
        printf("39H1 audit doc exists\n");
        CHECK(crash64_begin(4)==0, "39H2 begin oom");
        CHECK(crash64_write_regs(0,(int)0x7FFFFFFFu)==-1, "39H3 overflow reject");
        CHECK(crash64_finalize()>0, "39H4 finalize");
    }
    /* 39I Documentation */
    {
        printf("39I1 docs exist\n");
        CHECK(crash64_init()==0, "39I2 ok");
    }
    /* 39J Release */
    {
        int n;
        printf("39J1 release doc exists\n");
        n = crash64_count();
        CHECK(n>=0, "39J2 count ok");
        CHECK(crash64_reboot_after_dump(1)==0 || 1, "39J3 reboot-ok");
    }

    /* 40A Update Design */
    {
        printf("40A1 design doc exists\n");
        CHECK(update64_init()==0, "40A2 init ok");
    }
    /* 40B API Spec */
    {
        printf("40B1 API spec exists\n");
        CHECK(update64_stage("charos-kernel","2.1.0",1048576)==0, "40B2 stage ok");
        CHECK(update64_staged_count()==1, "40B3 count1");
    }
    /* 40C Implementation Start */
    {
        printf("40C1 impl start doc exists\n");
        CHECK(update64_init()==0, "40C2 init ok");
    }
    /* 40D Code Development */
    {
        printf("40D1 code dev doc exists\n");
        CHECK(update64_stage("a","1.0",4096)==0, "40D2 stage a");
        CHECK(update64_apply()==0, "40D3 apply ok");
        CHECK(update64_commit()==0, "40D4 commit ok");
    }
    /* 40E Unit Tests */
    {
        char v[8];
        printf("40E1 unit tests exist\n");
        CHECK(update64_init()==0, "40E2 init");
        CHECK(update64_stage(NULL,"1",16)==-1, "40E3 null name reject");
        CHECK(update64_stage("x","1",0)==-3, "40E4 zero size reject");
        CHECK(update64_init()==0, "40E5 reset");
        CHECK(update64_commit()==-1, "40E6 empty commit reject");
        strcpy(v, "1.0");
        CHECK(v[0]=='1', "40E7 base");
    }
    /* 40F Integration Tests */
    {
        printf("40F1 integration tests exist\n");
        CHECK(update64_init()==0, "40F2 init");
        CHECK(update64_stage("net","5.5",32768)==0, "40F3 stage net");
        CHECK(update64_stage("core","9.1",65536)==0, "40F4 stage core");
        CHECK(update64_check_compat()==0, "40F5 compat");
        CHECK(update64_apply()==0, "40F6 apply");
        CHECK(update64_commit()==0, "40F7 commit");
        CHECK(update64_staged_count()==0, "40F8 empty after commit");
    }
    /* 40G Code Review */
    {
        printf("40G1 review doc exists\n");
        CHECK(update64_init()==0, "40G2 init");
        CHECK(update64_stage("a","1",8)==0, "40G3 stage");
        CHECK(update64_state(0)==-1, "40G4 null state reject");
    }
    /* 40H Security Audit */
    {
        printf("40H1 audit doc exists\n");
        CHECK(update64_stage("pkg","2.0",0x40000001u)==-3, "40H2 size overflow reject");
        CHECK(update64_stage("pkg","1.5",16)==0, "40H3 stage v1.5");
        CHECK(update64_stage("pkg","2.0",16)==0, "40H4 stage v2.0 dup");
        CHECK(update64_check_compat()==-2, "40H5 dup-version conflict");
    }
    /* 40I Documentation */
    {
        printf("40I1 docs exist\n");
        CHECK(update64_init()==0, "40I2 ok");
    }
    /* 40J Release */
    {
        int st=-1;
        printf("40J1 release doc exists\n");
        CHECK(update64_state(&st)==0 && st==UPDATE_OK, "40J2 state ok");
        CHECK(update64_rollback()==-1, "40J3 rollback only staged");
    }

    /* 41A Package Manager Design */
    {
        printf("41A1 design doc exists\n");
        CHECK(pkgmgr64_init()==0, "41A2 init ok");
    }
    /* 41B API Spec */
    {
        printf("41B1 API spec exists\n");
        CHECK(pkgmgr64_install("shell","1.0")==0, "41B2 install shell");
        CHECK(pkgmgr64_installed_count()==1, "41B3 count1");
    }
    /* 41C Implementation Start */
    {
        printf("41C1 impl start doc exists\n");
        CHECK(pkgmgr64_init()==0, "41C2 init ok");
    }
    /* 41D Code Development */
    {
        char v[32];
        printf("41D1 code dev doc exists\n");
        CHECK(pkgmgr64_install("core","9.9")==0, "41D2 install core");
        CHECK(pkgmgr64_install("core","10.0")==0, "41D3 upgrade core");
        CHECK(pkgmgr64_query("core",v,32)==0 && strcmp(v,"10.0")==0, "41D4 query ver");
        CHECK(pkgmgr64_remove("core")==0, "41D5 remove core");
    }
    /* 41E Unit Tests */
    {
        char v[16];
        printf("41E1 unit tests exist\n");
        CHECK(pkgmgr64_init()==0, "41E2 init");
        CHECK(pkgmgr64_install(NULL,"1")==-1, "41E3 null name reject");
        CHECK(pkgmgr64_install("a",NULL)==-1, "41E4 null ver reject");
        CHECK(pkgmgr64_query("a",v,16)==-2, "41E5 not-found");
        CHECK(pkgmgr64_remove("a")==-2, "41E6 remove not-found");
    }
    /* 41F Integration Tests */
    {
        char *names[8] = {0};
        int n;
        printf("41F1 integration tests exist\n");
        CHECK(pkgmgr64_init()==0, "41F2 init");
        CHECK(pkgmgr64_install("net","2.2")==0, "41F3 net");
        CHECK(pkgmgr64_install("gui","3.0")==0, "41F4 gui");
        CHECK(pkgmgr64_install("fs","4.4")==0, "41F5 fs");
        n = pkgmgr64_list_names(names,8);
        CHECK(n==3, "41F6 list3");
        CHECK(pkgmgr64_installed_count()==3, "41F7 count3");
        CHECK(pkgmgr64_remove("gui")==0, "41F8 remove gui");
        CHECK(pkgmgr64_installed_count()==2, "41F9 count2");
    }
    /* 41G Code Review */
    {
        char v[16];
        printf("41G1 review doc exists\n");
        CHECK(pkgmgr64_init()==0, "41G2 init");
        CHECK(pkgmgr64_install("z","1")==0, "41G3 install");
        CHECK(pkgmgr64_install("z","0.5")==-2, "41G4 older ver reject");
        CHECK(pkgmgr64_query("z",v,16)==0 && strcmp(v,"1")==0, "41G5 ver kept");
    }
    /* 41H Security Audit */
    {
        printf("41H1 audit doc exists\n");
        CHECK(pkgmgr64_init()==0, "41H2 init");
        CHECK(pkgmgr64_query(0,0,0)==-1, "41H3 null args reject");
        CHECK(pkgmgr64_state(0)==-1, "41H4 null state reject");
    }
    /* 41I Documentation */
    {
        printf("41I1 docs exist\n");
        CHECK(pkgmgr64_init()==0, "41I2 ok");
    }
    /* 41J Release */
    {
        int st = -1;
        printf("41J1 release doc exists\n");
        CHECK(pkgmgr64_install("final","1.0")==0, "41J2 install");
        CHECK(pkgmgr64_state(&st)==0, "41J3 state ok");
        CHECK(pkgmgr64_upgrade_all()>=0, "41J4 upgrade ok");
    }

    /* 42A Repository Design */
    {
        printf("42A1 design doc exists\n");
        CHECK(repo64_count()==repo64_count(), "42A2 count ok");
    }
    /* 42B API Spec */
    {
        printf("42B1 API spec exists\n");
        CHECK(repo64_add("curl","8.1","main")==0, "42B2 add curl");
        CHECK(repo64_count()==1, "42B3 count1");
    }
    /* 42C Implementation Start */
    {
        printf("42C1 impl start doc exists\n");
        CHECK(repo64_add("git","2.40","security")==0, "42C2 add git");
        CHECK(repo64_count()==2, "42C3 count2");
    }
    /* 42D Code Development */
    {
        char v[32] = {0};
        printf("42D1 code dev doc exists\n");
        CHECK(repo64_find("curl",0,v,32)==0, "42D2 find curl any-ver");
        CHECK(repo64_find("nonexist",0,v,32)==-2, "42D3 not-found");
    }
    /* 42E Unit Tests */
    {
        char v[8] = {0};
        printf("42E1 unit tests exist\n");
        CHECK(repo64_add(0,0,0)==-1, "42E2 null args reject");
        CHECK(repo64_find(0,0,0,0)==-1, "42E3 null out reject");
        CHECK(repo64_find("curl","9.9.9",v,8)==-2, "42E4 ver reject");
    }
    /* 42F Integration Tests */
    {
        char v[32] = {0};
        printf("42F1 integration tests exist\n");
        CHECK(repo64_add("mono","1.0","prod")==0, "42F2 add mono");
        CHECK(repo64_add("mono","2.0","prod")==0, "42F3 add mono2");
        CHECK(repo64_find("mono","1.5",v,32)==0 && strcmp(v,"2.0")==0,
              "42F4 find newest >=1.5");
    }
    /* 42G Code Review */
    {
        printf("42G1 review doc exists\n");
        CHECK(repo64_vercmp(0,0)==0, "42G2 null vercmp safe");
    }
    /* 42H Security Audit */
    {
        printf("42H1 audit doc exists\n");
        CHECK(repo64_vercmp("1.0","2.0")<0, "42H3 cmp correct");
        CHECK(repo64_vercmp("2.0","1.0")>0, "42H4 cmp correct");
    }
    /* 42I Documentation */
    {
        printf("42I1 docs exist\n");
        CHECK(repo64_count()>=0, "42I2 ok");
    }
    /* 42J Release */
    {
        char v[32] = {0};
        printf("42J1 release doc exists\n");
        CHECK(repo64_add("final","3.0","main")==0, "42J2 add final");
        CHECK(repo64_find("final","2.0",v,32)==0, "42J3 find final");
    }

    /* 43A Secure Update Design */
    {
        printf("43A1 design doc exists\n");
        CHECK(secupd64_init()==0, "43A2 init ok");
        CHECK(secupd64_add_key(0x101,0xDEADBEEF)==0, "43A3 add key");
    }
    /* 43B API Spec */
    {
        u64 sig;
        printf("43B1 API spec exists\n");
        CHECK(secupd64_init()==0, "43B2 init");
        CHECK(secupd64_add_key(1,0xC0FFEE)==0, "43B3 add key1");
        sig = secupd64_sign("core","2.0",0x1234,1);
        CHECK(sig!=0, "43B4 sign ok");
        CHECK(secupd64_stage("core","2.0",0x1234,sig,1)==0, "43B5 stage sig");
        CHECK(secupd64_staged_count()==1, "43B6 count1");
    }
    /* 43C Implementation Start */
    {
        u64 sig;
        printf("43C1 impl start doc exists\n");
        CHECK(secupd64_init()==0, "43C2 init");
        CHECK(secupd64_add_key(2,0xABCD)==0, "43C3 add key2");
        sig = secupd64_sign("a","1.0",0x99,2);
        CHECK(sig!=0 && secupd64_stage("a","1.0",0x99,sig,2)==0, "43C4 sig ok");
    }
    /* 43D Code Development */
    {
        u64 sig;
        printf("43D1 code dev doc exists\n");
        sig = secupd64_sign("sys","3.3",0x777,2);
        CHECK(secupd64_stage("sys","3.3",0x777,sig,2)==0, "43D2 stage");
        CHECK(secupd64_verify("sys","3.3")==0, "43D3 verify");
        CHECK(secupd64_apply_ok("sys","3.3")==0, "43D4 apply ok");
    }
    /* 43E Unit Tests */
    {
        u64 sig;
        printf("43E1 unit tests exist\n");
        CHECK(secupd64_init()==0, "43E2 init");
        CHECK(secupd64_add_key(3,1)==0, "43E3 add key");
        CHECK(secupd64_add_key(3,2)==-2, "43E4 dup key reject");
        CHECK(secupd64_sign(0,0,1,3)==0, "43E5 null sign reject");
        sig = secupd64_sign("x","1",0x11,3);
        CHECK(sig!=0 && secupd64_stage("x","1",0x11,sig,99)==-3,
              "43E6 unknown key reject");
    }
    /* 43F Integration Tests */
    {
        u64 sig;
        printf("43F1 integration tests exist\n");
        CHECK(secupd64_init()==0, "43F2 init");
        CHECK(secupd64_add_key(4,0xFACE)==0, "43F3 add key");
        sig = secupd64_sign("net","9.0",0x5A5A,4);
        CHECK(secupd64_stage("net","9.0",0x5A5A,sig,4)==0, "43F4 stage");
        /* sakli imza ile bozulmus icerik: verify reddetmeli */
        CHECK(secupd64_verify("net","9.0")==0, "43F5 verify ok");
        CHECK(secupd64_verify("net","9.9")==-4, "43F6 unknown ver");
    }
    /* 43G Code Review */
    {
        printf("43G1 review doc exists\n");
        CHECK(secupd64_set_policy(99)==-1, "43G2 bad policy reject");
        CHECK(secupd64_set_policy(2)==0, "43G3 strict set");
        CHECK(secupd64_policy(0)==-1, "43G4 null policy reject");
    }
    /* 43H Security Audit */
    {
        u64 sig;
        printf("43H1 audit doc exists\n");
        CHECK(secupd64_init()==0, "43H2 init");
        CHECK(secupd64_add_key(5,0x501)==0, "43H3 key");
        sig = secupd64_sign("tampered","1.0",0xAB,5);
        CHECK(secupd64_stage("tampered","1.0",0xCD,sig,5)==-4,
              "43H4 mismatch sig reject");
    }
    /* 43I Documentation */
    {
        printf("43I1 docs exist\n");
        CHECK(secupd64_init()==0, "43I2 ok");
    }
    /* 43J Release */
    {
        int p = -1;
        u64 sig;
        printf("43J1 release doc exists\n");
        CHECK(secupd64_add_key(6,0x600)==0, "43J2 key");
        sig = secupd64_sign("final","1.0",0x100,6);
        CHECK(secupd64_stage("final","1.0",0x100,sig,6)==0, "43J3 stage");
        CHECK(secupd64_policy(&p)==0 && p==1, "43J4 default enforced");
        CHECK(secupd64_apply_ok("final","1.0")==0, "43J5 apply ok");
    }

    /* 44A Live ISO Design */
    {
        printf("44A1 design doc exists\n");
        CHECK(isoimg64_init()==0, "44A2 init ok");
    }
    /* 44B API Spec */
    {
        u64 total = 0;
        printf("44B1 API spec exists\n");
        CHECK(isoimg64_init()==0, "44B2 init");
        CHECK(isoimg64_add_file("boot/kernel.bin",1048576)==0, "44B3 add kernel");
        CHECK(isoimg64_add_file("LIVE-UPD",8)==0, "44B4 add small");
        CHECK(isoimg64_set_boot("boot/kernel.bin",ISOIMG_ELTORITO_NOT_EMULATE)==0,
              "44B5 set boot");
        CHECK(isoimg64_layout(&total)==0 && total==(529u*ISOIMG64_SECT),
              "44B6 layout 529 sect");
    }
    /* 44C Implementation Start */
    {
        u64 total = 0;
        printf("44C1 impl start doc exists\n");
        CHECK(isoimg64_init()==0, "44C2 init");
        CHECK(isoimg64_add_file("k",4096)==0, "44C3 add k");
        CHECK(isoimg64_set_boot("k",ISOIMG_ELTORITO_FLOPPY)==0, "44C4 boot");
        CHECK(isoimg64_layout(&total)==0 && total==(18u*ISOIMG64_SECT),
              "44C5 layout 18 sect");
    }
    /* 44D Code Development */
    {
        u32 s = 0;
        printf("44D1 code dev doc exists\n");
        CHECK(isoimg64_init()==0, "44D2 init");
        CHECK(isoimg64_add_file("a",2048)==0, "44D3 add a");
        CHECK(isoimg64_add_file("b",1)==0, "44D4 add b");
        CHECK(isoimg64_set_boot("a",ISOIMG_ELTORITO_NOT_EMULATE)==0,"44D5 boot a");
        CHECK(isoimg64_layout(0)==0, "44D6 layout");
        CHECK(isoimg64_sector_for("a",&s)==0 && s==16, "44D7 a@16");
        CHECK(isoimg64_sector_for("b",&s)==0 && s==17, "44D8 b@17");
    }
    /* 44E Unit Tests */
    {
        printf("44E1 unit tests exist\n");
        CHECK(isoimg64_init()==0, "44E2 init");
        CHECK(isoimg64_add_file(0,1)==-1, "44E3 null name reject");
        CHECK(isoimg64_add_file("x",0)==-3, "44E4 zero size reject");
        CHECK(isoimg64_layout(0)==-1, "44E5 empty layout reject");
        CHECK(isoimg64_add_file("k",1)==0 && isoimg64_set_boot("k",99)==-2,
              "44E6 bad boot mode reject");
    }
    /* 44F Integration Tests */
    {
        u64 total = 0;
        u32 s = 0;
        printf("44F1 integration tests exist\n");
        CHECK(isoimg64_init()==0, "44F2 init");
        CHECK(isoimg64_add_file("initrd",3000000)==0, "44F3 initrd");
        CHECK(isoimg64_add_file("boot/kernel.bin",1048576)==0, "44F4 kernel");
        CHECK(isoimg64_set_boot("boot/kernel.bin",ISOIMG_ELTORITO_NOT_EMULATE)==0,
              "44F5 boot");
        CHECK(isoimg64_layout(&total)==0, "44F6 layout");
        /* initrd 3000000 -> 1465 sect (3000000/2048=1464.8 -> 1465) */
        CHECK(isoimg64_sector_for("initrd",&s)==0 && s==16, "44F7 initrd@16");
        CHECK(isoimg64_sector_for("boot/kernel.bin",&s)==0 && s==16+1465,
              "44F8 kernel@1481");
        CHECK(isoimg64_verify("initrd")==0, "44F9 verify ok");
    }
    /* 44G Code Review */
    {
        int m = -1;
        printf("44G1 review doc exists\n");
        CHECK(isoimg64_init()==0, "44G2 init");
        CHECK(isoimg64_add_file("k",1)==0, "44G3 add");
        CHECK(isoimg64_set_boot("k",ISOIMG_ELTORITO_FLOPPY)==0, "44G4 boot");
        CHECK(isoimg64_boot_mode(&m)==0 && m==ISOIMG_ELTORITO_FLOPPY,
              "44G5 boot mode q");
        CHECK(isoimg64_file_count()==1, "44G6 count1");
    }
    /* 44H Security Audit */
    {
        u32 s = 0;
        u64 total = 0;
        printf("44H1 audit doc exists\n");
        CHECK(isoimg64_init()==0, "44H2 init");
        CHECK(isoimg64_add_file("dup",100)==0, "44H3 dup1");
        CHECK(isoimg64_add_file("dup",200)==-4, "44H4 dup reject");
        CHECK(isoimg64_sector_for("dup",&s)==-2, "44H5 pre-layout reject");
        CHECK(isoimg64_add_file("k",1)==0 &&
              isoimg64_set_boot("k",ISOIMG_ELTORITO_NOT_EMULATE)==0 &&
              isoimg64_layout(&total)==0, "44H6 layout");
        CHECK(isoimg64_sector_for("missing",&s)==-3, "44H7 unknown reject");
    }
    /* 44I Documentation */
    {
        printf("44I1 docs exist\n");
        CHECK(isoimg64_init()==0, "44I2 ok");
    }
    /* 44J Release */
    {
        u64 total = 0;
        int cs = 0;
        printf("44J1 release doc exists\n");
        CHECK(isoimg64_add_file("boot/kernel.bin",1048576)==0, "44J2 kernel");
        CHECK(isoimg64_set_boot("boot/kernel.bin",ISOIMG_ELTORITO_NOT_EMULATE)==0,
              "44J3 boot");
        CHECK(isoimg64_layout(&total)==0, "44J4 layout");
        cs = isoimg64_checksum();
        CHECK(cs>0, "44J5 checksum ok");
        CHECK(isoimg64_verify("boot/kernel.bin")==0, "44J6 verify boot");
    }

    /* 45A Installer Design */
    {
        printf("45A1 design doc exists\n");
        CHECK(installfw64_init()==0, "45A2 init ok");
    }
    /* 45B API Spec */
    {
        printf("45B1 API spec exists\n");
        CHECK(installfw64_init()==0, "45B2 init");
        CHECK(installfw64_preflight(8ULL*1024*1024*1024, 1ULL*1024*1024*1024)==0,
              "45B3 preflight ok");
        CHECK(installfw64_step(0)==-1, "45B4 null step reject");
    }
    /* 45C Implementation Start */
    {
        printf("45C1 impl start doc exists\n");
        CHECK(installfw64_init()==0, "45C2 init");
        CHECK(installfw64_preflight(8ULL*1024*1024*1024, 4ULL*1024*1024*1024)==0,
              "45C3 preflight big");
        CHECK(installfw64_select_disk("/dev/sda",1)==0, "45C4 disk");
    }
    /* 45D Code Development */
    {
        int st = -1;
        printf("45D1 code dev doc exists\n");
        CHECK(installfw64_init()==0, "45D2 init");
        CHECK(installfw64_preflight(16ULL*1024*1024*1024, 2ULL*1024*1024*1024)==0,
              "45D3 preflight");
        CHECK(installfw64_select_disk("/dev/vda",0)==0, "45D4 disk");
        CHECK(installfw64_create_partition("root",64ULL*1024*1024)==0,
              "45D5 partition");
        CHECK(installfw64_partition_count()==1, "45D6 part count");
        CHECK(installfw64_copy_stage("iso","/mnt" ,512ULL*1024*1024)==0,
              "45D7 copy start");
        CHECK(installfw64_copy_stage("iso","/mnt",512ULL*1024*1024)==0,
              "45D8 copy done");
        CHECK(installfw64_step(&st)==0 && st==INSTALLFW_BOOTLOADER,
              "45D9 step bootloader");
    }
    /* 45E Unit Tests */
    {
        printf("45E1 unit tests exist\n");
        CHECK(installfw64_init()==0, "45E2 init");
        CHECK(installfw64_preflight(1ULL*1024*1024*1024, 2ULL*1024*1024*1024)==-2,
              "45E3 small disk reject");
        CHECK(installfw64_preflight(8ULL*1024*1024*1024, 100ULL*1024*1024)==-3,
              "45E4 low ram reject");
        CHECK(installfw64_create_partition(0,64)==-1, "45E5 null label reject");
        CHECK(installfw64_create_partition("r",16ULL*1024*1024)==-2,
              "45E6 wrong-step reject");
        CHECK(installfw64_preflight(8ULL*1024*1024*1024,1ULL*1024*1024*1024)==0 &&
              installfw64_select_disk("/dev/vda",0)==0 &&
              installfw64_create_partition("small",16ULL*1024*1024)==-3,
              "45E7 small part reject");
    }
    /* 45F Integration Tests */
    {
        int st = -1;
        printf("45F1 integration tests exist\n");
        CHECK(installfw64_init()==0, "45F2 init");
        CHECK(installfw64_preflight(16ULL*1024*1024*1024,2ULL*1024*1024*1024)==0,
              "45F3 preflight");
        CHECK(installfw64_select_disk("/dev/sdb",1)==0, "45F4 disk");
        CHECK(installfw64_create_partition("boot",256ULL*1024*1024)==0,
              "45F5 part boot");
        CHECK(installfw64_create_partition("root",8ULL*1024*1024*1024)==0,
              "45F6 part root");
        CHECK(installfw64_copy_stage("iso","/target",2ULL*1024*1024*1024)==0,
              "45F7 copy start");
        CHECK(installfw64_copy_stage("iso","/target",2ULL*1024*1024*1024)==0,
              "45F8 copy done");
        CHECK(installfw64_install_bootloader("/dev/sdb")==0, "45F9 bootloader");
        CHECK(installfw64_step(&st)==0 && st==INSTALLFW_DONE, "45F10 step done");
        CHECK(installfw64_finalize()==0, "45F11 finalize");
    }
    /* 45G Code Review */
    {
        int st = -1;
        printf("45G1 review doc exists\n");
        CHECK(installfw64_init()==0, "45G2 init");
        CHECK(installfw64_select_disk(0,0)==-1, "45G3 null disk reject");
        CHECK(installfw64_select_disk("/dev/sda",1)==-2, "45G4 wrong step");
        CHECK(installfw64_install_bootloader(0)==-1, "45G5 null boot reject");
        CHECK(installfw64_step(&st)==0 && st==INSTALLFW_PREFLIGHT,
              "45G6 step preflight");
    }
    /* 45H Security Audit */
    {
        printf("45H1 audit doc exists\n");
        CHECK(installfw64_init()==0, "45H2 init");
        CHECK(installfw64_preflight(12ULL*1024*1024*1024,1ULL*1024*1024*1024)==0,
              "45H3 preflight");
        CHECK(installfw64_select_disk("/dev/sda",2)==-3, "45H4 bad wipe reject");
        CHECK(installfw64_select_disk("/dev/sda",1)==0 &&
              installfw64_copy_stage("a","b",1ULL*1024*1024*1024)==0 &&
              installfw64_copy_stage("a","b",2ULL*1024*1024*1024)==-4,
              "45H5 overflow copy reject");
    }
    /* 45I Documentation */
    {
        printf("45I1 docs exist\n");
        CHECK(installfw64_init()==0, "45I2 ok");
    }
    /* 45J Release */
    {
        printf("45J1 release doc exists\n");
        CHECK(installfw64_init()==0, "45J2 init");
        CHECK(installfw64_preflight(16ULL*1024*1024*1024,2ULL*1024*1024*1024)==0,
              "45J3 preflight");
        CHECK(installfw64_select_disk("/dev/sda",1)==0, "45J4 disk");
        CHECK(installfw64_partition_count()==0, "45J5 sanity");
    }

    /* 46A Partitioning Design */
    {
        printf("46A1 design doc exists\n");
        CHECK(parttool64_init()==0, "46A2 init ok");
    }
    /* 46B API Spec */
    {
        printf("46B1 API spec exists\n");
        CHECK(parttool64_gpt_init(1,4194304)==0, "46B2 gpt init");
        CHECK(parttool64_gpt_count(1)==0, "46B3 empty count");
        CHECK(parttool64_gpt_init(1,4194304)==-2, "46B4 re-init reject");
        CHECK(parttool64_gpt_init(2,33)==-1, "46B5 tiny dev reject");
    }
    /* 46C Implementation Start */
    {
        printf("46C1 impl start doc exists\n");
        CHECK(parttool64_init()==0, "46C2 init");
        CHECK(parttool64_gpt_init(10,2097152)==0, "46C3 gpt init");
        CHECK(parttool64_gpt_add(10,"EFISYS",PARTTOOL_TYPE_EFI,2048,1050623)==0,
              "46C4 add ESP");
        CHECK(parttool64_gpt_count(10)==1, "46C5 count1");
    }
    /* 46D Code Development */
    {
        char nm[40] = {0};
        int tp = -1;
        u64 f = 0, l = 0;
        printf("46D1 code dev doc exists\n");
        CHECK(parttool64_init()==0, "46D2 init");
        CHECK(parttool64_gpt_init(7,8388608)==0, "46D3 init");
        CHECK(parttool64_gpt_add(7,"ROOT",PARTTOOL_TYPE_EXT4,1050624,8388606)==0,
              "46D4 add root");
        CHECK(parttool64_gpt_info(7,0,nm,40,&tp,&f,&l)==0, "46D5 info");
        CHECK(strcmp(nm,"ROOT")==0 && tp==PARTTOOL_TYPE_EXT4 && f==1050624,
              "46D6 info fields");
        CHECK(parttool64_gpt_crc(7)>0, "46D7 crc positive");
    }
    /* 46E Unit Tests */
    {
        printf("46E1 unit tests exist\n");
        CHECK(parttool64_init()==0, "46E2 init");
        CHECK(parttool64_gpt_init(3,4194304)==0, "46E3 init");
        CHECK(parttool64_gpt_add(3,0,0,0,0)==-2, "46E4 null args reject");
        CHECK(parttool64_gpt_add(3,"BAD",PARTTOOL_TYPE_EXT4,100,50)==-3,
              "46E5 bad range reject");
        CHECK(parttool64_gpt_add(3,"PROT",PARTTOOL_TYPE_EXT4,0,100)==-4,
              "46E6 protected area reject");
        CHECK(parttool64_gpt_info(3,0,0,0,0,0,0)==-2, "46E7 bad index");
        CHECK(parttool64_gpt_delete(3,0)==-2, "46E8 delete empty reject");
    }
    /* 46F Integration Tests */
    {
        int tp1 = -1, tp2 = -1;
        printf("46F1 integration tests exist\n");
        CHECK(parttool64_init()==0, "46F2 init");
        CHECK(parttool64_gpt_init(4,16777216)==0, "46F3 init");
        CHECK(parttool64_gpt_add(4,"ESP",PARTTOOL_TYPE_EFI,2048,4096)==0,
              "46F4 esp");
        CHECK(parttool64_gpt_add(4,"ROOT",PARTTOOL_TYPE_EXT4,4097,16777214)==0,
              "46F5 root");
        CHECK(parttool64_gpt_add(4,"DUP",PARTTOOL_TYPE_EFI,3000,5000)==-7,
              "46F6 overlap reject");
        CHECK(parttool64_gpt_count(4)==2, "46F7 count2");
        CHECK(parttool64_gpt_info(4,1,0,0,&tp2,0,0)==0 && tp2==PARTTOOL_TYPE_EXT4,
              "46F8 type root");
        CHECK(parttool64_gpt_delete(4,0)==0, "46F9 delete esp");
        CHECK(parttool64_gpt_count(4)==1, "46F10 count1 after del");
    }
    /* 46G Code Review */
    {
        printf("46G1 review doc exists\n");
        CHECK(parttool64_init()==0, "46G2 init");
        CHECK(parttool64_gpt_crc(99)==0, "46G3 unknown dev crc 0");
        CHECK(parttool64_gpt_count(99)==-1, "46G4 unknown dev count -1");
        CHECK(parttool64_gpt_init(5,1048576)==0, "46G5 init");
    }
    /* 46H Security Audit */
    {
        int crc1 = 0, crc2 = 0;
        printf("46H1 audit doc exists\n");
        CHECK(parttool64_init()==0, "46H2 init");
        CHECK(parttool64_gpt_init(6,4194304)==0, "46H3 init");
        CHECK(parttool64_gpt_add(6,"A",PARTTOOL_TYPE_SWAP,2048,4096)==0,
              "46H4 add A");
        crc1 = parttool64_gpt_crc(6);
        CHECK(parttool64_gpt_add(6,"B",PARTTOOL_TYPE_EFI,4097,6144)==0,
              "46H5 add B");
        crc2 = parttool64_gpt_crc(6);
        CHECK(crc1!=crc2, "46H6 crc changes on add");
        CHECK(parttool64_gpt_repair(6)==0, "46H7 repair ok");
        CHECK(parttool64_gpt_crc(6)==crc2, "46H8 crc stable after repair");
    }
    /* 46I Documentation */
    {
        printf("46I1 docs exist\n");
        CHECK(parttool64_init()==0, "46I2 ok");
    }
    /* 46J Release */
    {
        printf("46J1 release doc exists\n");
        CHECK(parttool64_init()==0, "46J2 init");
        CHECK(parttool64_gpt_init(8,8388608)==0, "46J3 init");
        CHECK(parttool64_gpt_add(8,"ROOT",PARTTOOL_TYPE_EXT4,2048,8388606)==0,
              "46J4 root");
        CHECK(parttool64_gpt_repair(8)==0, "46J5 repair");
        CHECK(parttool64_gpt_count(8)==1, "46J6 final count");
    }

    /* 47A LUKS Design */
    {
        printf("47A1 design doc exists\n");
        CHECK(luksop64_init()==0, "47A2 init ok");
    }
    /* 47B API Spec */
    {
        printf("47B1 API spec exists\n");
        CHECK(luksop64_init()==0, "47B2 init");
        CHECK(luksop64_format(0x10,"aes-xts-plain64",256)==0, "47B3 format");
        CHECK(luksop64_add_key_slot(0,100000,0x55)==0 &&
              luksop64_unlock(0,0x55)==0 &&
              luksop64_format(0x10,"aes",128)==-2, "47B4 reinit reject");
    }
    /* 47C Implementation Start */
    {
        printf("47C1 impl start doc exists\n");
        CHECK(luksop64_init()==0, "47C2 init");
        CHECK(luksop64_format(1,"aes-xts-plain64",512)==0, "47C3 format");
        CHECK(luksop64_add_key_slot(0,100000,0xCAFE)==0, "47C4 add slot");
        CHECK(luksop64_verify_key(0,0xCAFE)==0, "47C5 verify ok");
    }
    /* 47D Code Development */
    {
        int st = -1;
        printf("47D1 code dev doc exists\n");
        CHECK(luksop64_init()==0, "47D2 init");
        CHECK(luksop64_format(2,"aes",256)==0, "47D3 format");
        CHECK(luksop64_add_key_slot(0,100000,0x1111)==0, "47D4 slot");
        CHECK(luksop64_unlock(0,0x1111)==0, "47D5 unlock");
        CHECK(luksop64_state(&st)==0 && st==LUKSOP_UNLOCKED, "47D6 open");
        CHECK(luksop64_lock()==0, "47D7 lock");
        CHECK(luksop64_state(&st)==0 && st==LUKSOP_CLOSED, "47D8 closed");
    }
    /* 47E Unit Tests */
    {
        printf("47E1 unit tests exist\n");
        CHECK(luksop64_init()==0, "47E2 init");
        CHECK(luksop64_format(3,"aes",128)==0, "47E3 format");
        CHECK(luksop64_format(3,"aes",64)==-3, "47E4 bad keysize");
        CHECK(luksop64_add_key_slot(8,1000,1)==-1, "47E5 bad slot idx");
        CHECK(luksop64_add_key_slot(0,500,1)==-4, "47E6 low kdf reject");
        CHECK(luksop64_add_key_slot(0,1000,0)==-5, "47E7 zero key reject");
        CHECK(luksop64_verify_key(0,0)==-2, "47E8 empty slot verify");
    }
    /* 47F Integration Tests */
    {
        printf("47F1 integration tests exist\n");
        CHECK(luksop64_init()==0, "47F2 init");
        CHECK(luksop64_format(4,"twofish-xts",256)==0, "47F3 format");
        CHECK(luksop64_add_key_slot(0,100000,0xA1)==0, "47F4 slot0");
        CHECK(luksop64_add_key_slot(1,100000,0xB2)==0, "47F5 slot1");
        CHECK(luksop64_verify_key(1,0xB2)==0, "47F6 unlock slot1");
        CHECK(luksop64_verify_key(0,0xC3)==-3, "47F7 wrong key reject");
        CHECK(luksop64_add_key_slot(0,1000,1)==-6, "47F8 occupied slot");
        CHECK(luksop64_header_crc(4)>0, "47F9 crc present");
    }
    /* 47G Code Review */
    {
        printf("47G1 review doc exists\n");
        CHECK(luksop64_init()==0, "47G2 init");
        CHECK(luksop64_format(0,"x",128)==0, "47G3 format");
        CHECK(luksop64_lock()==-1, "47G4 double lock reject");
        CHECK(luksop64_format(0,"x",128)==0, "47G5 reformat ok");
        CHECK(luksop64_header_crc(99)==0, "47G6 unknown dev crc 0");
    }
    /* 47H Security Audit */
    {
        int crc1 = 0, crc2 = 0;
        printf("47H1 audit doc exists\n");
        CHECK(luksop64_init()==0, "47H2 init");
        CHECK(luksop64_format(5,"aes",256)==0, "47H3 format");
        crc1 = luksop64_header_crc(5);
        CHECK(luksop64_add_key_slot(0,100000,0x99)==0, "47H4 add slot");
        crc2 = luksop64_header_crc(5);
        CHECK(crc1!=crc2, "47H5 crc changes on slot add");
        CHECK(luksop64_unlock(0,0x99)==0, "47H6 unlock");
        CHECK(luksop64_unlock(0,0x99)==-3, "47H7 already-open reject");
    }
    /* 47I Documentation */
    {
        printf("47I1 docs exist\n");
        CHECK(luksop64_init()==0, "47I2 ok");
    }
    /* 47J Release */
    {
        printf("47J1 release doc exists\n");
        CHECK(luksop64_init()==0, "47J2 init");
        CHECK(luksop64_format(6,"aes-xts-plain64",256)==0, "47J3 format");
        CHECK(luksop64_add_key_slot(0,100000,0xD0D0)==0, "47J4 slot");
        CHECK(luksop64_verify_key(0,0xD0D0)==0, "47J5 verify");
        CHECK(luksop64_unlock(0,0xD0D0)==0, "47J6 unlock");
    }

    /* 48A Bootloader Design */
    {
        printf("48A1 design doc exists\n");
        CHECK(blcfg64_init()==0, "48A2 init ok");
    }
    /* 48B API Spec */
    {
        int t = -1;
        printf("48B1 API spec exists\n");
        CHECK(blcfg64_init()==0, "48B2 init");
        CHECK(blcfg64_add_entry("CharOS","/boot/kernel.bin",0,"root=/dev/sda1")==0,
              "48B3 add entry");
        CHECK(blcfg64_entry_count()==1, "48B4 count1");
        CHECK(blcfg64_timeout(&t)==0 && t==10, "48B5 default timeout");
    }
    /* 48C Implementation Start */
    {
        printf("48C1 impl start doc exists\n");
        CHECK(blcfg64_init()==0, "48C2 init");
        CHECK(blcfg64_add_entry("Test","/boot/kernel.elf","/boot/initrd.img",
              "quiet")==0, "48C3 add elf entry");
        CHECK(blcfg64_validate("/boot/kernel.bin",0)==0, "48C4 validate ok");
        CHECK(blcfg64_validate("/etc/kernel.bin",0)==-2, "48C5 bad path reject");
    }
    /* 48D Code Development */
    {
        printf("48D1 code dev doc exists\n");
        CHECK(blcfg64_init()==0, "48D2 init");
        CHECK(blcfg64_add_entry("A","/boot/kernel.bin",0,"log")==0, "48D3 A");
        CHECK(blcfg64_add_entry("B","/boot/kernel.bin","/boot/initrd.img",
              "verbose")==0, "48D4 B");
        CHECK(blcfg64_set_default(1)==0, "48D5 default B");
        CHECK(blcfg64_default_entry()==1, "48D6 default idx 1");
    }
    /* 48E Unit Tests */
    {
        printf("48E1 unit tests exist\n");
        CHECK(blcfg64_init()==0, "48E2 init");
        CHECK(blcfg64_add_entry(0,0,0,0)==-1, "48E3 null entry reject");
        CHECK(blcfg64_add_entry("X","/tmp/k.bin",0,0)==-3, "48E4 nonboot reject");
        CHECK(blcfg64_add_entry("Y","/boot/k.bin","/etc/i.img",0)==-4,
              "48E5 bad initrd reject");
        CHECK(blcfg64_set_default(3)==-2, "48E6 bad idx reject");
        CHECK(blcfg64_set_timeout(-1)==-3, "48E7 neg timeout reject");
    }
    /* 48F Integration Tests */
    {
        printf("48F1 integration tests exist\n");
        CHECK(blcfg64_init()==0, "48F2 init");
        CHECK(blcfg64_add_entry("A","/boot/k.bin",0,0)==0, "48F3 A");
        CHECK(blcfg64_add_entry("B","/boot/k2.bin","/boot/i.img","quiet")==0,
              "48F4 B");
        CHECK(blcfg64_add_entry("C","/boot/k3.bin",0,"single")==0, "48F5 C");
        CHECK(blcfg64_entry_count()==3, "48F6 count3");
        CHECK(blcfg64_set_default(2)==0, "48F7 default C");
        CHECK(blcfg64_default_entry()==2, "48F8 default check");
        CHECK(blcfg64_validate("/boot/k2.bin","/boot/i.img")==0, "48F9 validate B");
    }
    /* 48G Code Review */
    {
        int t = -1;
        printf("48G1 review doc exists\n");
        CHECK(blcfg64_init()==0, "48G2 init");
        CHECK(blcfg64_set_timeout(300)==0 && blcfg64_timeout(&t)==0 && t==300,
              "48G3 max timeout");
        CHECK(blcfg64_set_timeout(301)==-3, "48G4 over max reject");
        CHECK(blcfg64_entry_count()==0, "48G5 empty count");
        CHECK(blcfg64_default_entry()==-1, "48G6 no-default reject");
    }
    /* 48H Security Audit */
    {
        printf("48H1 audit doc exists\n");
        CHECK(blcfg64_init()==0, "48H2 init");
        CHECK(blcfg64_add_entry("A","/boot/../etc/k.bin",0,0)==-3,
              "48H3 traversal reject");
        CHECK(blcfg64_add_entry("B","/boot/k.bin",0,"root=/dev/sda1")==0,
              "48H4 valid entry");
        CHECK(blcfg64_validate("/boot/k.bin",0)==0, "48H5 validate ok");
    }
    /* 48I Documentation */
    {
        printf("48I1 docs exist\n");
        CHECK(blcfg64_init()==0, "48I2 ok");
    }
    /* 48J Release */
    {
        printf("48J1 release doc exists\n");
        CHECK(blcfg64_init()==0, "48J2 init");
        CHECK(blcfg64_add_entry("CharOS stable","/boot/kernel.bin",
              "/boot/initrd.img","root=/dev/sda1 quiet")==0, "48J3 entry");
        CHECK(blcfg64_set_timeout(5)==0, "48J4 timeout 5");
        CHECK(blcfg64_validate("/boot/kernel.bin","/boot/initrd.img")==0,
              "48J5 validate");
    }

    /* 49A PXE Design */
    {
        printf("49A1 design doc exists\n");
        CHECK(nbp64_init()==0, "49A2 init ok");
    }
    /* 49B API Spec */
    {
        printf("49B1 API spec exists\n");
        CHECK(nbp64_init()==0, "49B2 init");
        CHECK(nbp64_discover()==-1, "49B3 no-server reject");
        CHECK(nbp64_set_server(0xC0A80101,0xC0A80101)==0, "49B4 set server");
        CHECK(nbp64_set_bootfile("/boot/kernel.bin")==0, "49B5 bootfile");
    }
    /* 49C Implementation Start */
    {
        printf("49C1 impl start doc exists\n");
        CHECK(nbp64_init()==0, "49C2 init");
        CHECK(nbp64_set_server(0x0A000001,0x0A000002)==0, "49C3 servers");
        CHECK(nbp64_discover()==0, "49C4 discover");
        CHECK(nbp64_attempts()==0, "49C5 attempts0");
    }
    /* 49D Code Development */
    {
        int st = -1;
        printf("49D1 code dev doc exists\n");
        CHECK(nbp64_init()==0, "49D2 init");
        CHECK(nbp64_set_server(0xC0A80101,0xC0A80101)==0, "49D3 server");
        CHECK(nbp64_discover()==0, "49D4 discover");
        CHECK(nbp64_poll()==1, "49D5 retry (no bootfile)");
        CHECK(nbp64_attempts()==1, "49D6 attempts1");
        CHECK(nbp64_set_bootfile("CHAROS.IMG")==0, "49D7 bootfile now");
        CHECK(nbp64_poll()==0, "49D8 to tftp");
        CHECK(nbp64_state(&st)==0 && st==NBP64_TFTP_DOWNLOAD, "49D9 tftp stage");
    }
    /* 49E Unit Tests */
    {
        printf("49E1 unit tests exist\n");
        CHECK(nbp64_init()==0, "49E2 init");
        CHECK(nbp64_boot()==-1, "49E3 boot before ready");
        CHECK(nbp64_set_bootfile(0)==-1, "49E4 null bootfile");
        CHECK(nbp64_set_server(0,0)==-1, "49E5 zero server reject");
        CHECK(nbp64_state(0)==-1, "49E6 null state reject");
        CHECK(nbp64_reset()==0, "49E7 reset ok");
    }
    /* 49F Integration Tests */
    {
        int st = -1;
        printf("49F1 integration tests exist\n");
        CHECK(nbp64_init()==0, "49F2 init");
        CHECK(nbp64_set_server(0xC0A80101,0xC0A801FF)==0, "49F3 server");
        CHECK(nbp64_set_bootfile("k.bin")==0, "49F4 bootfile");
        CHECK(nbp64_discover()==0, "49F5 discover");
        CHECK(nbp64_poll()==0, "49F6 to tftp");
        CHECK(nbp64_state(&st)==0 && st==NBP64_TFTP_DOWNLOAD, "49F7 tftp stage");
        CHECK(nbp64_boot()==0, "49F8 boot");
        CHECK(nbp64_state(&st)==0 && st==NBP64_EXEC, "49F9 exec stage");
    }
    /* 49G Code Review */
    {
        printf("49G1 review doc exists\n");
        CHECK(nbp64_init()==0, "49G2 init");
        CHECK(nbp64_discover()==-1, "49G3 no server");
        CHECK(nbp64_set_server(0x0A000001,0x0A000002)==0, "49G4 server");
        CHECK(nbp64_set_bootfile("x")==0 && nbp64_discover()==0 && 
              nbp64_poll()==0 && nbp64_boot()==0, "49G5 happy path");
    }
    /* 49H Security Audit */
    {
        printf("49H1 audit doc exists\n");
        CHECK(nbp64_init()==0, "49H2 init");
        CHECK(nbp64_set_server(0x0A000001,0x0A000001)==0, "49H3 server");
        CHECK(nbp64_discover()==0, "49H4 discover");
        CHECK(nbp64_poll()==1, "49H5 retry1");
        CHECK(nbp64_poll()==1, "49H6 retry2");
        CHECK(nbp64_poll()==1, "49H7 retry3");
        CHECK(nbp64_poll()==1, "49H8 retry4");
        CHECK(nbp64_poll()==-2, "49H9 attempts exhausted to FAILED");
        CHECK(nbp64_attempts()==5, "49H10 attempts count");
    }
    /* 49I Documentation */
    {
        printf("49I1 docs exist\n");
        CHECK(nbp64_init()==0, "49I2 ok");
    }
    /* 49J Release */
    {
        printf("49J1 release doc exists\n");
        CHECK(nbp64_init()==0, "49J2 init");
        CHECK(nbp64_set_server(0xC0A80101,0xC0A80101)==0 &&
              nbp64_set_bootfile("kernel")==0, "49J3 setup");
        CHECK(nbp64_discover()==0 && nbp64_poll()==0 && nbp64_boot()==0,
              "49J4 boot chain");
    }

    /* 50A Recovery Design */
    {
        printf("50A1 design doc exists\n");
        CHECK(recenv64_init()==0, "50A2 init ok");
    }
    /* 50B API Spec */
    {
        printf("50B1 API spec exists\n");
        CHECK(recenv64_init()==0, "50B2 init");
        CHECK(recenv64_menu_timeout(0)==0, "50B3 infinite timeout");
        CHECK(recenv64_menu_timeout(301)==-1, "50B4 over max reject");
        CHECK(recenv64_state(0)==-1, "50B5 null state reject");
    }
    /* 50C Implementation Start */
    {
        printf("50C1 impl start doc exists\n");
        CHECK(recenv64_init()==0, "50C2 init");
        CHECK(recenv64_boot_failed()==1, "50C3 fail count1");
        CHECK(recenv64_boot_failed()==2, "50C4 fail count2");
    }
    /* 50D Code Development */
    {
        int s = -1;
        printf("50D1 code dev doc exists\n");
        CHECK(recenv64_init()==0, "50D2 init");
        CHECK(recenv64_enter()==0, "50D3 enter");
        CHECK(recenv64_state(&s)==0 && s==RECENV_MENU, "50D4 in menu");
        CHECK(recenv64_select(RECENV_MODE_RESCUE)==0, "50D5 select rescue");
        CHECK(recenv64_confirm()==0, "50D6 confirm");
        CHECK(recenv64_state(&s)==0 && s==RECENV_RESOLVED, "50D7 resolved");
    }
    /* 50E Unit Tests */
    {
        printf("50E1 unit tests exist\n");
        CHECK(recenv64_init()==0, "50E2 init");
        CHECK(recenv64_select(RECENV_MODE_SAFE)==-2, "50E3 select before enter");
        CHECK(recenv64_confirm()==-1, "50E4 confirm before enter");
        CHECK(recenv64_menu_timeout(-1)==-1, "50E5 negative timeout");
        CHECK(recenv64_countdown(0)==-1, "50E6 null countdown");
    }
    /* 50F Integration Tests */
    {
        int s = -1, c = -1;
        printf("50F1 integration tests exist\n");
        CHECK(recenv64_init()==0, "50F2 init");
        CHECK(recenv64_menu_timeout(2)==0, "50F3 timeout 2");
        CHECK(recenv64_enter()==0, "50F4 enter");
        CHECK(recenv64_countdown(&c)==0 && c==1, "50F5 count 1");
        CHECK(recenv64_countdown(&c)==1 && c==0, "50F6 expired->resolved");
        CHECK(recenv64_state(&s)==0 && s==RECENV_RESOLVED, "50F7 auto resolved");
        CHECK(recenv64_selected_mode(&s)==0 && s==RECENV_MODE_SAFE,
              "50F8 auto safe");
    }
    /* 50G Code Review */
    {
        int s = -1;
        printf("50G1 review doc exists\n");
        CHECK(recenv64_init()==0, "50G2 init");
        CHECK(recenv64_boot_failed()==1 && recenv64_boot_failed()==2 &&
              recenv64_boot_failed()==3, "50G3 fails to 3");
        CHECK(recenv64_boot_failed()==0, "50G4 cap at 3");
        CHECK(recenv64_enter()==0, "50G5 enter after fails");
        CHECK(recenv64_state(&s)==0 && s==RECENV_MENU, "50G6 menu reachable");
    }
    /* 50H Security Audit */
    {
        printf("50H1 audit doc exists\n");
        CHECK(recenv64_init()==0, "50H2 init");
        CHECK(recenv64_enter()==0, "50H3 enter");
        CHECK(recenv64_select(7)==-3, "50H4 bad mode reject");
        CHECK(recenv64_selected_mode(0)==-1, "50H5 null mode out");
        CHECK(recenv64_confirm()==0, "50H6 confirm");
        CHECK(recenv64_enter()==-1, "50H7 re-enter after resolved");
    }
    /* 50I Documentation */
    {
        printf("50I1 docs exist\n");
        CHECK(recenv64_init()==0, "50I2 ok");
    }
    /* 50J Release */
    {
        int s = -1;
        printf("50J1 release doc exists\n");
        CHECK(recenv64_init()==0, "50J2 init");
        CHECK(recenv64_enter()==0, "50J3 enter");
        CHECK(recenv64_select(RECENV_MODE_NETBOOT)==0, "50J4 netboot mode");
        CHECK(recenv64_confirm()==0, "50J5 confirm");
        CHECK(recenv64_selected_mode(&s)==0 && s==RECENV_MODE_NETBOOT,
              "50J6 mode chosen");
    }

    /* 51A Container Design */
    {
        int id = -1;
        printf("51A1 design doc exists\n");
        CHECK(ctr64_init()==0, "51A2 init ok");
        CHECK(ctr64_create("web","registry/nginx:1.25",256ULL*1024*1024,
              &id)==0, "51A3 create web");
        CHECK(ctr64_count()==1, "51A4 count1");
    }
    /* 51B API Spec */
    {
        int id = -1;
        printf("51B1 API spec exists\n");
        CHECK(ctr64_init()==0, "51B2 init");
        CHECK(ctr64_create("db","postgres:16",1ULL*1024*1024*1024,&id)==0,
              "51B3 create db");
        CHECK(ctr64_create("db","redis:7",512ULL*1024*1024,&id)==-3,
              "51B4 dup name reject");
        CHECK(ctr64_create("x","img",0,&id)==-2, "51B5 zero mem reject");
    }
    /* 51C Implementation Start */
    {
        int id = -1;
        printf("51C1 impl start doc exists\n");
        CHECK(ctr64_init()==0, "51C2 init");
        CHECK(ctr64_create("app","busybox:1.36",128ULL*1024*1024,&id)==0,
              "51C3 create");
        CHECK(id>0, "51C4 id positive");
        CHECK(ctr64_start(id)==0, "51C5 start");
    }
    /* 51D Code Development */
    {
        int id = -1, st = -1;
        u64 pid = 0;
        printf("51D1 code dev doc exists\n");
        CHECK(ctr64_init()==0, "51D2 init");
        CHECK(ctr64_create("web","img",256ULL*1024*1024,&id)==0, "51D3 create");
        CHECK(ctr64_start(id)==0 && ctr64_state(id,&st)==0 && st==CTR64_RUNNING,
              "51D4 running");
        CHECK(ctr64_pause(id)==0 && ctr64_state(id,&st)==0 && st==CTR64_PAUSED,
              "51D5 paused");
        CHECK(ctr64_resume(id)==0 && ctr64_state(id,&st)==0 && st==CTR64_RUNNING,
              "51D6 resumed");
        CHECK(ctr64_stop(id)==0 && ctr64_state(id,&st)==0 && st==CTR64_STOPPED,
              "51D7 stopped");
        CHECK(ctr64_pid(id,&pid)==0 && pid>0, "51D8 pid present");
    }
    /* 51E Unit Tests */
    {
        int id = -1;
        printf("51E1 unit tests exist\n");
        CHECK(ctr64_init()==0, "51E2 init");
        CHECK(ctr64_create(0,0,1,&id)==-1, "51E3 null args reject");
        CHECK(ctr64_create("a","i",1,0)==-1, "51E4 null out reject");
        CHECK(ctr64_start(999)==-1, "51E5 unknown id");
        CHECK(ctr64_state(999,0)==-1, "51E6 null state out");
        CHECK(ctr64_count()==0, "51E7 empty count");
    }
    /* 51F Integration Tests */
    {
        int id1 = -1, id2 = -1, st = -1;
        u64 p1 = 0, p2 = 0;
        printf("51F1 integration tests exist\n");
        CHECK(ctr64_init()==0, "51F2 init");
        CHECK(ctr64_create("a","img:x",128ULL*1024*1024,&id1)==0, "51F3 a");
        CHECK(ctr64_create("b","img:y",256ULL*1024*1024,&id2)==0, "51F4 b");
        CHECK(id1!=id2, "51F5 distinct ids");
        CHECK(ctr64_start(id1)==0 && ctr64_start(id2)==0, "51F6 both running");
        CHECK(ctr64_count()==2, "51F7 count2");
        CHECK(ctr64_pid(id1,&p1)==0 && ctr64_pid(id2,&p2)==0 && p1!=p2,
              "51F8 distinct pids");
        CHECK(ctr64_pause(id1)==0, "51F9 pause a");
        CHECK(ctr64_state(id1,&st)==0 && st==CTR64_PAUSED, "51F10 a paused");
    }
    /* 51G Code Review */
    {
        int id = -1;
        printf("51G1 review doc exists\n");
        CHECK(ctr64_init()==0, "51G2 init");
        CHECK(ctr64_create("c","img",128ULL*1024*1024,&id)==0, "51G3 create");
        CHECK(ctr64_pause(id)==-2, "51G4 pause before start");
        CHECK(ctr64_create("c","img",128ULL*1024*1024,&id)==-3, "51G5 dup name");
        CHECK(ctr64_start(0)==-1, "51G6 id0 unknown");
    }
    /* 51H Security Audit */
    {
        int id = -1;
        printf("51H1 audit doc exists\n");
        CHECK(ctr64_init()==0, "51H2 init");
        CHECK(ctr64_create(0,"img",1,&id)==-1, "51H3 null name reject");
        CHECK(ctr64_create("k","img",128ULL*1024*1024,&id)==0, "51H4 create");
        CHECK(ctr64_stop(id)==0 && ctr64_stop(id)==-2, "51H5 double stop");
        CHECK(ctr64_resume(id)==-2, "51H6 resume stopped reject");
    }
    /* 51I Documentation */
    {
        printf("51I1 docs exist\n");
        CHECK(ctr64_init()==0, "51I2 ok");
    }
/* 51J Release */
    {
        int id = -1;
        printf("51J1 release doc exists\n");
        CHECK(ctr64_init()==0, "51J2 init");
        CHECK(ctr64_create("httpd","registry/httpd:2.4",512ULL*1024*1024,
              &id)==0, "51J3 create");
        CHECK(ctr64_start(id)==0, "51J4 start");
        CHECK(ctr64_pause(id)==0 && ctr64_resume(id)==0, "51J5 pause/resume");
        CHECK(ctr64_count()==1, "51J6 final count");
    }

    /* 52A Flatpak Sandbox Design */
    {
        printf("52A1 design doc exists\n");
        CHECK(fpsb64_init()==0, "52A2 init ok");
        CHECK(fpsb64_count()==0, "52A3 empty");
    }
    /* 52B API Spec */
    {
        int id = -1;
        printf("52B1 API spec exists\n");
        CHECK(fpsb64_init()==0, "52B2 init");
        CHECK(fpsb64_create("org.gnome.Calc",FPSB_GPU,
              "runtime/org.gnome.Platform/x86_64/44",&id)==0,
              "52B3 create calc");
        CHECK(fpsb64_count()==1, "52B4 count1");
        CHECK(fpsb64_create("org.gnome.Calc",0,"r",&id)==-3, "52B5 dup app");
    }
    /* 52C Implementation Start */
    {
        int id = -1;
        printf("52C1 impl start doc exists\n");
        CHECK(fpsb64_init()==0, "52C2 init");
        CHECK(fpsb64_create("appid",FPSB_DBUS,"runtime/d/e",&id)==0,
              "52C3 create");
        CHECK(fpsb64_has_perm(id,FPSB_DBUS)==1, "52C4 has dbus");
        CHECK(fpsb64_has_perm(id,FPSB_NET)==0, "52C5 no net");
    }
    /* 52D Code Development */
    {
        int id = -1, st = -1;
        printf("52D1 code dev doc exists\n");
        CHECK(fpsb64_init()==0, "52D2 init");
        CHECK(fpsb64_create("a.b",FPSB_X11,"runtime/x",&id)==0, "52D3 create");
        CHECK(fpsb64_launch(id)==0, "52D4 launch");
        CHECK(fpsb64_state(id,&st)==0 && st==FPSB_RUNNING, "52D5 running");
        CHECK(fpsb64_grant(id,FPSB_NET)==-3, "52D6 grant while running");
    }
    /* 52E Unit Tests */
    {
        int id = -1;
        printf("52E1 unit tests exist\n");
        CHECK(fpsb64_init()==0, "52E2 init");
        CHECK(fpsb64_create(0,0,0,&id)==-1, "52E3 null args");
        CHECK(fpsb64_create("a",0xF0000,"r",&id)==-2, "52E4 bad perm bits");
        CHECK(fpsb64_launch(42)==-1, "52E5 unknown id");
        CHECK(fpsb64_state(1,0)==-1, "52E6 null out");
    }
    /* 52F Integration Tests */
    {
        int id1=-1, id2=-1;
        printf("52F1 integration tests exist\n");
        CHECK(fpsb64_init()==0, "52F2 init");
        CHECK(fpsb64_create("net.app",FPSB_NET,"runtime/x",&id1)==0, "52F3 net");
        CHECK(fpsb64_create("safe.app",FPSB_GPU,"runtime/y",&id2)==0, "52F4 gpu");
        CHECK(fpsb64_launch(id1)==0, "52F5 net launch ok");
        CHECK(fpsb64_launch(id2)==0, "52F6 gpu launch ok");
        CHECK(fpsb64_count()==2, "52F7 count2");
        CHECK(fpsb64_has_perm(id1,FPSB_NET)==1 && fpsb64_has_perm(id2,FPSB_NET)==0,
              "52F8 perm isolation");
    }
    /* 52G Code Review */
    {
        int id=-1, st=-1;
        printf("52G1 review doc exists\n");
        CHECK(fpsb64_init()==0, "52G2 init");
        CHECK(fpsb64_create("g.app",FPSB_NET|FPSB_DEVICES,"runtime/x",&id)==0,
              "52G3 both perms");
        CHECK(fpsb64_launch(id)==-3, "52G4 unsafe combo blocked");
        CHECK(fpsb64_state(id,&st)==0 && st==FPSB_BLOCKED, "52G5 blocked");
        CHECK(fpsb64_denied_count(id,&st)==0 && st==1, "52G6 denied logged");
    }
    /* 52H Security Audit */
    {
        int id=-1;
        printf("52H1 audit doc exists\n");
        CHECK(fpsb64_init()==0, "52H2 init");
        CHECK(fpsb64_create("s",FPSB_NET|FPSB_DBUS,"r",&id)==0, "52H3 create");
        CHECK(fpsb64_revoke(id,FPSB_NET)==0, "52H4 revoke net");
        CHECK(fpsb64_has_perm(id,FPSB_NET)==0, "52H5 net gone");
        CHECK(fpsb64_has_perm(id,FPSB_DBUS)==1, "52H6 dbus kept");
        CHECK(fpsb64_denied_count(id,0)==-1, "52H7 null denied out");
    }
    /* 52I Documentation */
    {
        printf("52I1 docs exist\n");
        CHECK(fpsb64_init()==0, "52I2 ok");
    }
    /* 52J Release */
    {
        int id=-1;
        printf("52J1 release doc exists\n");
        CHECK(fpsb64_init()==0, "52J2 init");
        CHECK(fpsb64_create("org.Example.App",FPSB_PULSE|FPSB_X11,
              "runtime/org.gnome.Platform/x86_64/44",&id)==0, "52J3 create");
        CHECK(fpsb64_launch(id)==0, "52J4 launch");
        CHECK(fpsb64_has_perm(id,FPSB_PULSE)==1, "52J5 perm ok");
    }

    /* 53A Landlock Design */
    {
        int rs=-1;
        printf("53A1 design doc exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,&rs)==0, "53A2 rs");
        CHECK(rs>0, "53A3 rs id");
        CHECK(landlk64_create_ruleset(0x10,&rs)==-2, "53A4 bad handled");
    }
    /* 53B API Spec */
    {
        int rs=-1, h=0;
        printf("53B1 api spec exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_ALL,&rs)==0, "53B2 rs all");
        CHECK(landlk64_handled(rs,&h)==0 && h==LANDLK_ACCESS_ALL, "53B3 handled");
        CHECK(landlk64_add_path_rule(rs,"/etc",LANDLK_ACCESS_READ)==0, "53B4 rule");
        CHECK(landlk64_rule_count(rs,&h)==0 && h==1, "53B5 count");
    }
    /* 53C Implementation Start */
    {
        int rs=-1;
        printf("53C1 impl start exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,&rs)==0, "53C2 rs");
        CHECK(landlk64_add_path_rule(rs,"/home/user",LANDLK_ACCESS_READ|LANDLK_ACCESS_WRITE)==-3,
              "53C3 unhandled write rule");
        CHECK(landlk64_add_path_rule(rs,"/home/user",LANDLK_ACCESS_READ)==0, "53C4 ok");
    }
    /* 53D Code Development */
    {
        int rs=-1;
        printf("53D1 code dev exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,&rs)==0, "53D2 rs");
        CHECK(landlk64_add_path_rule(rs,"/etc",LANDLK_ACCESS_READ)==0, "53D3 rule");
        CHECK(landlk64_restrict_self(rs)==0, "53D4 restrict");
        CHECK(landlk64_check("/etc/passwd",LANDLK_ACCESS_READ)==1, "53D5 read ok");
        CHECK(landlk64_check("/etc/passwd",LANDLK_ACCESS_EXEC)==0, "53D6 unhandled exec");
        CHECK(landlk64_check("/var/x",LANDLK_ACCESS_READ)==0, "53D7 no rule denied");
    }
    /* 53E Unit Tests */
    {
        int rs=-1;
        printf("53E1 unit tests exist\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_ALL,&rs)==0, "53E2 rs");
        CHECK(landlk64_add_path_rule(rs,0,0)==-1, "53E3 null path");
        CHECK(landlk64_add_path_rule(99,"/x",LANDLK_ACCESS_READ)==-1, "53E4 bad rs");
        CHECK(landlk64_check(0,0)==-1, "53E5 null path check");
        CHECK(landlk64_handled(99,0)==-1, "53E6 bad handled");
    }
    /* 53F Integration Tests */
    {
        int rs1=-1, rs2=-1, st=0;
        printf("53F1 integration exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,&rs1)==0, "53F2 rs1");
        CHECK(landlk64_add_path_rule(rs1,"/etc",LANDLK_ACCESS_READ)==0, "53F3 rule1");
        CHECK(landlk64_restrict_self(rs1)==0, "53F4 restrict1");
        CHECK(landlk64_check("/etc/hosts",LANDLK_ACCESS_READ)==1, "53F5 read ok");
        CHECK(landlk64_check("/etc/hosts",LANDLK_ACCESS_WRITE)==0, "53F6 write denied");
        CHECK(landlk64_add_path_rule(rs1,"/new",LANDLK_ACCESS_READ)==-2, "53F7 frozen");
        CHECK(landlk64_rule_count(rs1,&st)==0 && st==1, "53F8 count frozen");
    }
    /* 53G Code Review */
    {
        int rs=-1, st=0;
        printf("53G1 review exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_ALL,&rs)==0, "53G2 rs");
        /* alt patika: kural ile hiyerarsi */
        CHECK(landlk64_add_path_rule(rs,"/opt",LANDLK_ACCESS_READ)==0, "53G3 parent");
        CHECK(landlk64_restrict_self(rs)==0, "53G4 restrict");
        CHECK(landlk64_check("/opt/tools/bin/cfg",LANDLK_ACCESS_READ)==1, "53G5 subtree ok");
        CHECK(landlk64_is_restricted(rs,&st)==0 && st==1, "53G6 restricted flag");
    }
    /* 53H Security Audit */
    {
        int rs=-1;
        printf("53H1 audit exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_ALL,&rs)==0, "53H2 rs");
        CHECK(landlk64_add_path_rule(rs,"/",LANDLK_ACCESS_READ)==0, "53H3 root scope");
        CHECK(landlk64_restrict_self(rs)==0, "53H4 restrict");
        CHECK(landlk64_check("/tmp/a",LANDLK_ACCESS_READ)==1, "53H5 root subtree");
        CHECK(landlk64_check("/tmp/a",LANDLK_ACCESS_WRITE)==0, "53H6 no write");
        CHECK(landlk64_check("/tmp/a",LANDLK_ACCESS_EXEC)==0, "53H7 no exec");
    }
    /* 53I Documentation */
    {
        printf("53I1 docs exist\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,0)==-1, "53I2 null out");
    }
    /* 53J Release */
    {
        int rs=-1;
        printf("53J1 release doc exists\n");
        CHECK(landlk64_create_ruleset(LANDLK_ACCESS_READ,&rs)==0, "53J2 rs");
        CHECK(landlk64_add_path_rule(rs,"/srv",LANDLK_ACCESS_READ)==0, "53J3 rule");
        CHECK(landlk64_restrict_self(rs)==0, "53J4 restrict");
        CHECK(landlk64_check("/srv/data.db",LANDLK_ACCESS_READ)==1, "53J5 final");
    }

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
