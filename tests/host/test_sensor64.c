/* 49J: hwmon64/cputemp64/fan64/battstat64/sysmon64/prom64/smart64/
 *      logrotate64/health64 host testi.
 * Calistirma: make test-sensor64
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

static int check_ok(void) { return 0; }
static int check_bad(void) { return -1; }

int main(void) {
    int v = 0;
    char out[1024];

    CHECK(hwmon64_add("coretemp", HWMON64_TEMP) == 0, "49A ekle");
    CHECK(hwmon64_add("fan1", HWMON64_FAN) == 0, "49A ekle2");
    CHECK(hwmon64_add("x", 99) != 0, "49A tur red");
    CHECK(hwmon64_update("coretemp", 65) == 0, "49A guncelle");
    CHECK(hwmon64_read("coretemp", &v) == 0 && v == 65, "49A oku");
    CHECK(hwmon64_read("yok", 0) != 0, "49A yok red");

    CHECK(cputemp64_update(0, 100, 35) == 0, "49B cekirdek0");
    CHECK(cputemp64_update(1, 100, 25) == 0, "49B cekirdek1");
    CHECK(cputemp64_core(0) == 65, "49B oku0");
    CHECK(cputemp64_core(1) == 75, "49B oku1");
    CHECK(cputemp64_max() == 75, "49B azami");
    CHECK(cputemp64_update(0, 50, 99) != 0, "49B ters red");
    CHECK(cputemp64_core(99) < -999, "49B cekirdek red");

    CHECK(fan64_set_target(0, 70) == 0, "49C hedef");
    CHECK(fan64_tick(0, 80) > 30, "49C hizlan");
    CHECK(fan64_tick(0, 40) < 100, "49C yavasla");
    CHECK(fan64_pwm(0) >= 0 && fan64_pwm(0) <= 100, "49C aralik");
    CHECK(fan64_curve(20) == 20, "49C dusuk");
    CHECK(fan64_curve(55) == 60, "49C orta");
    CHECK(fan64_curve(90) == 100, "49C yuksek");

    CHECK(battstat64_snapshot(40, 0, 15000, 50000) == 0, "49D gorunum");
    CHECK(battstat64_pct() == 40, "49D yuzde");
    CHECK(battstat64_minutes() == 80, "49D bitis");
    CHECK(battstat64_snapshot(40, 1, 15000, 50000) == 0, "49D sarj");
    CHECK(battstat64_minutes() == 120, "49D dolum");
    CHECK(battstat64_snapshot(200, 0, 1, 1) != 0, "49D aralik red");

    sysmon64_tick(1000, 3000, 1ULL << 30);
    sysmon64_tick(1500, 3500, 1ULL << 30);
    CHECK(sysmon64_cpu_pct() == 50, "49E cpu");
    CHECK(sysmon64_mem() == 1ULL << 30, "49E bellek");
    sysmon64_io(100, 200, 300, 400);
    sysmon64_tick(1500, 3500, 0);
    CHECK(sysmon64_cpu_pct() == 0, "49E sifir delta");

    CHECK(prom64_gauge("cpu_yuk", "CPU%%", 42) == 0, "49F gosterge");
    CHECK(prom64_inc("cpu_yuk") == 0, "49F artir");
    CHECK(prom64_inc("yeni_sayac") == 0, "49F otomatik");
    {
        int n = prom64_render(out, sizeof(out));
        CHECK(n > 0, "49F bicim");
        CHECK(strstr(out, "cpu_yuk 43") != 0, "49F deger");
        CHECK(strstr(out, "# TYPE cpu_yuk gauge") != 0, "49F tur");
    }

    CHECK(smart64_add(5, 100, 100, 10, 0) == 0, "49G oznitelik");
    CHECK(smart64_add(194, 60, 55, 0, 45) == 0, "49G sicaklik");
    CHECK(smart64_failing() == 0, "49G temiz");
    CHECK(smart64_health() == 1, "49G saglikli");
    CHECK(smart64_temp() == 45, "49G derece");
    CHECK(smart64_add(197, 5, 5, 10, 3) == 0, "49G supheli");
    CHECK(smart64_failing() == 1, "49G bozuk");
    CHECK(smart64_health() == 0, "49G sagliksiz");

    CHECK(logrotate64_add("sistem", 1000, 3) == 0, "49H ekle");
    CHECK(logrotate64_write("sistem", 600) == 0, "49H yaz");
    CHECK(logrotate64_generations("sistem") == 0, "49H henuz");
    CHECK(logrotate64_write("sistem", 600) == 0, "49H tasma");
    CHECK(logrotate64_generations("sistem") == 1, "49H kusak");
    CHECK(logrotate64_write("yok", 1) != 0, "49H kayitsiz red");

    CHECK(health64_add("disk", check_ok) == 0, "49I ekle");
    CHECK(health64_add("ag", check_bad) == 0, "49I ekle2");
    CHECK(health64_run() == 1, "49I 1 basarisiz");
    CHECK(health64_status() == 2, "49I maske");
    CHECK(health64_add(0, 0) != 0, "49I null red");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
