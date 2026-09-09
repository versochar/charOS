/* 42J: init64/unit64/sockact64/journald64/loginctl64/cgroup64/timer64/
 *      boottarget64/rescue64 host testi.
 * Calistirma: make test-init64
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
    int ids[8];
    char svc[32], inner[8][32], msg[128];
    int prio = -1, aid = -1;
    u64 lim = 0;

    CHECK(init64_add_service("ag") == 0, "42A ekle");
    CHECK(init64_add_service("net") == 0, "42A ekle2");
    CHECK(init64_state("ag") == INIT64_STOPPED, "42A duruyor");
    CHECK(init64_start("ag") == 0, "42A baslat");
    CHECK(init64_state("ag") == INIT64_RUNNING, "42A calisiyor");
    CHECK(init64_count_state(INIT64_RUNNING) == 1, "42A sayac");
    CHECK(init64_stop("ag") == 0, "42A durdur");
    CHECK(init64_state("yok") != INIT64_RUNNING, "42A yok");

    {
        const char *u = "[Unit]\nDescription=Ag servisi\n"
                        "After=net, db\nWants=log\n"
                        "[Service]\nExecStart=/sbin/ag --fg\n";
        struct unit64 un;
        CHECK(unit64_parse(u, &un) == 0, "42B ayristir");
        CHECK(!strcmp(un.desc, "Ag servisi"), "42B aciklama");
        CHECK(!strcmp(un.exec, "/sbin/ag --fg"), "42B exec");
        CHECK(un.nafter == 2 && !strcmp(un.after[1], "db"), "42B after");
        CHECK(un.nwants == 1 && !strcmp(un.wants[0], "log"), "42B wants");
        CHECK(unit64_parse("[Service]\n", &un) != 0, "42B execsiz red");
    }

    CHECK(sockact64_add("/run/ag.sock", "ag") == 0, "42C ekle");
    CHECK(sockact64_connection("/run/ag.sock", svc, sizeof(svc)) == 0 &&
          !strcmp(svc, "ag"), "42C tetikle");
    CHECK(sockact64_pending() == 1, "42C kuyruk");
    CHECK(sockact64_connection("/run/yok.sock", 0, 0) != 0,
          "42C eslesme yok");

    CHECK(journald64_write(3, "ag", "basladi") == 0, "42D yaz");
    CHECK(journald64_write(4, "ag", "uyari") == 0, "42D yaz2");
    CHECK(journald64_write(6, "net", "hazir") == 0, "42D yaz3");
    CHECK(journald64_count("ag") == 2, "42D sayac");
    CHECK(journald64_query("ag", 0, &prio, msg, sizeof(msg)) == 0 &&
          prio == 3 && !strcmp(msg, "basladi"), "42D sorgu");
    CHECK(journald64_query("ag", 5, 0, 0, 0) != 0, "42D asim red");
    CHECK(journald64_vacuum(2) == 1, "42D buda");
    CHECK(journald64_write(99, "x", "y") != 0, "42D prio red");

    {
        int s1 = loginctl64_new_session("mitsune", "seat0", 100);
        int s2 = loginctl64_new_session("misafir", "seat0", 101);
        CHECK(s1 > 0 && s2 > s1, "42E oturumlar");
        CHECK(loginctl64_activate(s1) == 0, "42E etkinlestir");
        CHECK(loginctl64_active(&aid) == 0 && aid == s1, "42E etkin oku");
        CHECK(loginctl64_list(ids, 8) == 2, "42E liste");
        CHECK(loginctl64_terminate(s2) == 0, "42E sonlandir");
        CHECK(loginctl64_list(ids, 8) == 1 && ids[0] == s1, "42E liste2");
        CHECK(loginctl64_activate(9999) != 0, "42E yok red");
    }

    CHECK(cgroup64_mkdir("/sys/fs/cgroup/ag") == 0, "42F olustur");
    CHECK(cgroup64_mkdir("goreceli") != 0, "42F mutlak sart");
    CHECK(cgroup64_set_limit("/sys/fs/cgroup/ag", "memory", 1ULL << 30) ==
          0, "42F limit");
    CHECK(cgroup64_get_limit("/sys/fs/cgroup/ag", "memory", &lim) == 0 &&
          lim == (1ULL << 30), "42F limit oku");
    CHECK(cgroup64_set_limit("/sys/fs/cgroup/ag", "bogus", 1) != 0,
          "42F denetleyici red");
    CHECK(cgroup64_add_task("/sys/fs/cgroup/ag", 100) == 0, "42F gorev");
    CHECK(cgroup64_add_task("/sys/fs/cgroup/ag", 100) == 0,
          "42F cift kayit sorunsuz");
    CHECK(cgroup64_tasks("/sys/fs/cgroup/ag") == 1, "42F sayac");

    CHECK(timer64_add("saatlik", "log-clean", 3600) == 0, "42G ekle");
    CHECK(timer64_add("dakikalik", "senk", 60) == 0, "42G ekle2");
    CHECK(timer64_tick(30, inner, 8) == 0, "42G erken ates yok");
    CHECK(timer64_tick(60, inner, 8) == 1 && !strcmp(inner[0], "senk"),
          "42G vade");
    CHECK(timer64_next_in(61) == 59, "42G sonraki vade");
    (void)msg;

    CHECK(boottarget64_add("ag", "net, log") == 0, "42H hedef");
    CHECK(boottarget64_add("net", "") == 0, "42H yaprak");
    CHECK(boottarget64_add("log", "") == 0, "42H yaprak2");
    {
        char plan[8][32];
        int n = boottarget64_plan("ag", plan, 8);
        CHECK(n == 3, "42H plan adedi");
        CHECK(!strcmp(plan[2], "ag"), "42H hedef en son");
        CHECK(!strcmp(plan[0], "net") || !strcmp(plan[0], "log"),
              "42H bagimlilik once");
    }
    CHECK(boottarget64_set_default("ag") == 0, "42H varsayilan");
    CHECK(boottarget64_plan(0, inner, 8) == 3, "42H varsayilan plan");
    CHECK(boottarget64_add("a", "b") == 0 && boottarget64_add("b", "a") == 0,
          "42H dongu kur");
    CHECK(boottarget64_plan("a", inner, 8) != 0, "42H dongu red");

    CHECK(!rescue64_active(), "42I normal");
    CHECK(rescue64_enter("disk hatasi") == 0, "42I gir");
    CHECK(rescue64_active() && rescue64_shell_ok(), "42I kabuk");
    CHECK(rescue64_enter("x") != 0, "42I cift giris red");
    CHECK(rescue64_exit() == 0 && !rescue64_active(), "42I cik");
    CHECK(rescue64_exit() != 0, "42I disarida cikis red");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
