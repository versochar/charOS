/* 32.5: GERÇEK kernel/drivers/thermal.c testi (aynı dosya derlenir).
 * Donanım çıktısı (serial/vga) stub'lanır; regülatör mantığı gerçektir.
 * Calistirma: make test-thermal32
 */
#include <stdio.h>
#include "drivers/thermal.h"
#include "core/power.h"

/* Donanım stub'ları (yalnızca host bağı için) */
void serial_puts(const char* s) { (void)s; }
void serial_puthex(unsigned int v) { (void)v; }
void vga_puts(const char* s) { (void)s; }

static int fails = 0;
#define CHECK(c, msg) do { \
    if (!(c)) { printf("FAIL %s\n", msg); fails++; } \
    else { printf("PASS %s\n", msg); } \
} while (0)

static unsigned int mock_temp = 90;
static unsigned int mock_sensor(void) { return mock_temp; }

int main(void) {
    /* 32.3: histerezis bandı salınımı yutar */
    thermal_init();
    for (int i = 0; i < 4; i++) thermal_tick(70);
    CHECK(thermal_get_pstate() == 0, "32.3 bant-salınım-yok");

    /* 32.3: kritik throttle (her kritik ortalama sayaçı artırır) */
    thermal_init();
    for (int i = 0; i < 4; i++) thermal_tick(100);
    CHECK(thermal_get_pstate() == 3 && thermal_trips() == 4, "32.3 kritik");

    /* 32.3: aralık dışı reddi */
    CHECK(thermal_tick(200) == 3, "32.3 red-durum-korunur");

    /* 32.3: enjekte sensör (ilk örnek baskın, 4 çağrıda pstate 3) */
    thermal_init();
    thermal_set_sensor(mock_sensor);
    for (int i = 0; i < 4; i++) thermal_dynamic_idle();
    CHECK(thermal_get_pstate() == 3 && thermal_last_temp() == 90,
          "32.3 sensor");

    /* 32.3: sensörsüz model tahminci (tam meşgul -> 73, ilk ort tek örnek) */
    thermal_init();
    power_init();
    for (int i = 0; i < 4; i++) power_note_tick(0);
    thermal_dynamic_idle();
    CHECK(thermal_last_temp() == 73, "32.3 model");

    /* 32.3: öztest */
    CHECK(thermal_selftest() == 0, "32.3 selftest");

    /* 32.9: deterministik tekrar */
    thermal_init();
    for (int i = 0; i < 4; i++) thermal_tick(80);
    int p1 = thermal_get_pstate();
    thermal_init();
    for (int i = 0; i < 4; i++) thermal_tick(80);
    CHECK(thermal_get_pstate() == p1 && p1 == 3, "32.9 deterministik");

    if (fails) { printf("SONUC: %d FAIL\n", fails); return 1; }
    printf("SONUC: TUMU PASS\n");
    return 0;
}
