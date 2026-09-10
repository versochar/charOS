/* 36.3: PCM halka tamponu mantığı.
 * Aynı dosya çekirdekte (freestanding) ve host testinde derlenir.
 */
#include "drivers/pcm.h"
#include "core/verify.h"

/* 36.3: derleme-zamanı kanıtı */
STATIC_ASSERT(PCM_CAP > 0 && PCM_CAP <= 4096);

static int16_t buf[PCM_CAP];
static uint32_t head = 0; /* okuma */
static uint32_t tail = 0; /* yazma */
static uint32_t used = 0;
static uint32_t under = 0;
static uint32_t over = 0;

void pcm_init(void) {
    head = 0; tail = 0; used = 0;
    under = 0; over = 0;
    for (int i = 0; i < PCM_CAP; i++) buf[i] = 0;
}

int pcm_write(const int16_t* s, uint32_t n) {
    uint32_t w = 0;
    if (REQUIRE(s != 0, 0xE701) != 0) return -1;
    while (w < n && used < PCM_CAP) {
        buf[tail] = s[w];
        tail = (tail + 1) % PCM_CAP;
        used++;
        w++;
    }
    if (w < n) over++; /* taşan kısım sayılır, yazılan korunur */
    return (int)w;
}

int pcm_read(int16_t* out, uint32_t n) {
    uint32_t r = 0;
    if (REQUIRE(out != 0, 0xE702) != 0) return -1;
    while (r < n && used > 0) {
        out[r] = buf[head];
        head = (head + 1) % PCM_CAP;
        used--;
        r++;
    }
    if (r < n) under++;
    return (int)r;
}

uint32_t pcm_avail(void) { return used; }
uint32_t pcm_free(void) { return PCM_CAP - used; }
uint32_t pcm_underruns(void) { return under; }
uint32_t pcm_overruns(void) { return over; }

int pcm_selftest(void) {
    int16_t w[4] = {100, 200, 300, 400};
    int16_t r[4] = {0, 0, 0, 0};
    pcm_init();
    if (pcm_write(w, 4) != 4) return -1;
    if (pcm_avail() != 4) return -2;
    if (pcm_read(r, 4) != 4) return -3;
    for (int i = 0; i < 4; i++) {
        if (r[i] != w[i]) return -4; /* sıra korunur */
    }
    /* sarmal: kapasite-üstü yazma kısmi + sayaç */
    pcm_init();
    {
        int16_t big[PCM_CAP + 10];
        for (int i = 0; i < PCM_CAP + 10; i++) big[i] = (int16_t)i;
        if (pcm_write(big, PCM_CAP + 10) != PCM_CAP) return -5;
        if (pcm_overruns() != 1) return -6;
    }
    if (pcm_read(r, 4) != 4 || r[0] != 0 || r[3] != 3) return -7;
    if (pcm_read(r, 0) != 0) return -8;
    if (pcm_write(0, 1) != -1) return -9;
    if (pcm_read(0, 1) != -1) return -10;
    return 0;
}
