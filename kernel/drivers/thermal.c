#include "drivers/thermal.h"
#include "drivers/vga.h"
#include "drivers/serial.h"
#include "core/power.h"
#include "core/verify.h"

/* 32.3: Histerezisli termal regülatör.
 * - Örneklerin 4'lü kayan ortalaması alınır (ani sıçrama yutulur).
 * - Yükselme eşiği 75, düşme eşiği 60 (salınım engeli).
 * - 95+ kritik: en düşük pstate + sayaç (throttle).
 * Sıcaklık kaynağı enjekte edilir; QEMU'da DTS olmadığı için çekirdek
 * yük-tabanlı model tahminci kullanır (belgelendi, sensör değil).
 */

static int current_pstate = 0; /* P0 = max */

#define THERMAL_AVG_N 4
#define THERMAL_UP 75
#define THERMAL_DOWN 60
#define THERMAL_CRIT 95

static uint32_t (*sensor_fn)(void) = 0;
static uint32_t samples[THERMAL_AVG_N];
static uint32_t nsamples = 0;
static uint32_t last_temp = 45;
static uint32_t trip_count = 0;

/* 32.3: derleme-zamanı kanıtı */
STATIC_ASSERT(THERMAL_UP > THERMAL_DOWN);
STATIC_ASSERT(THERMAL_CRIT > THERMAL_UP);

void thermal_init(void) {
    current_pstate = 0;
    sensor_fn = 0;
    nsamples = 0;
    last_temp = 45;
    trip_count = 0;
    for (int i = 0; i < THERMAL_AVG_N; i++) samples[i] = 45;
}

void thermal_set_sensor(uint32_t (*fn)(void)) {
    sensor_fn = fn;
}

static uint32_t model_temp(void) {
    /* 32.3: yük-tabanlı tahmin (40 + meşgul% / 3 → tam yükte ~73) */
    uint32_t busy = 100 - power_idle_pct();
    return 40 + busy / 3;
}

uint32_t thermal_get_temp(void) {
    return last_temp;
}

uint32_t thermal_last_temp(void) {
    return last_temp;
}

uint32_t thermal_trips(void) {
    return trip_count;
}

void thermal_set_pstate(int p) {
    if (p < 0) p = 0;
    if (p > THERMAL_PSTATE_MAX - 1) p = THERMAL_PSTATE_MAX - 1;
    current_pstate = p;
    serial_puts("[28F] P-state="); serial_puthex(p); serial_puts("\n");
}

int thermal_get_pstate(void) { return current_pstate; }

int thermal_tick(uint32_t temp) {
    uint32_t avg = 0;
    if (REQUIRE(temp <= 125, 0xE301) != 0) return current_pstate;
    samples[nsamples % THERMAL_AVG_N] = temp;
    nsamples++; /* 32.4-fix: sayaç hep ilerler (halka indeksi donmasın); n min() ile sınırlı */
    {
        uint32_t n = nsamples < THERMAL_AVG_N ? nsamples : THERMAL_AVG_N;
        uint32_t sum = 0;
        for (uint32_t i = 0; i < n; i++) sum += samples[i];
        avg = n ? sum / n : temp;
    }
    last_temp = avg;
    if (avg >= THERMAL_CRIT) {
        trip_count++;
        current_pstate = THERMAL_PSTATE_MAX - 1;
        return current_pstate;
    }
    if (avg >= THERMAL_UP && current_pstate < THERMAL_PSTATE_MAX - 1) {
        current_pstate++;
    } else if (avg <= THERMAL_DOWN && current_pstate > 0) {
        current_pstate--;
    }
    return current_pstate;
}

static uint32_t thermal_sample(void) {
    uint32_t t = sensor_fn ? sensor_fn() : model_temp();
    thermal_tick(t);
    return last_temp;
}

void thermal_dynamic_idle(void) {
    /* 32.4: eski davranış regülatör üzerinden (sıcak->düşük, serin->P0 eğilimi) */
    thermal_sample();
}

int thermal_selftest(void) {
    thermal_init();
    if (REQUIRE(thermal_get_pstate() == 0, 0xE302) != 0) return -1;
    /* ısıt: 4x80 -> ortalama 80, her tick bir kademe -> pstate 3 */
    for (int i = 0; i < 4; i++) thermal_tick(80);
    if (REQUIRE(thermal_get_pstate() == THERMAL_PSTATE_MAX - 1, 0xE303) != 0)
        return -2;
    /* soğut: histerezis bandı (60-75) salınımı yutar, 6x50 -> pstate 0 */
    for (int i = 0; i < 6; i++) thermal_tick(50);
    if (REQUIRE(thermal_get_pstate() == 0, 0xE304) != 0) return -3;
    /* kritik: 4x100 -> ortalama 100 -> throttle + sayaç */
    for (int i = 0; i < 4; i++) thermal_tick(100);
    if (REQUIRE(thermal_get_pstate() == THERMAL_PSTATE_MAX - 1, 0xE305) != 0)
        return -4;
    if (REQUIRE(thermal_trips() > 0, 0xE306) != 0) return -5;
    if (REQUIRE(thermal_tick(200) == thermal_get_pstate(), 0xE307) != 0)
        return -6; /* aralık dışı reddedilir, durum korunur */
    thermal_init();
    serial_puts("[28F] thermal [PASS]\n");
    vga_puts("[28F] thermal [PASS]\n");
    return 0;
}
