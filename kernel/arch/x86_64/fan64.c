/* 49C: fan control — hedef sicaklik + kademeli PWM + dogrusal egri. */
#include "arch/x86_64/longmode.h"

#define FAN64_MAX 8
#define FAN64_STEP 5

static int fan64_target[FAN64_MAX];
static int fan64_pwm_v[FAN64_MAX];
static int fan64_ready = 0;

static void fan64_ensure(void) {
    int i;
    if (fan64_ready) return;
    for (i = 0; i < FAN64_MAX; i++) {
        fan64_target[i] = 60;
        fan64_pwm_v[i] = 30;
    }
    fan64_ready = 1;
}

int fan64_set_target(int fan, int temp_c) {
    fan64_ensure();
    if (fan < 0 || fan >= FAN64_MAX || temp_c < 0 || temp_c > 110)
        return -1;
    fan64_target[fan] = temp_c;
    return 0;
}

/* Hedefe kademeli yaklas; donus guncel PWM. */
int fan64_tick(int fan, int temp_c) {
    fan64_ensure();
    if (fan < 0 || fan >= FAN64_MAX) return -1;
    if (temp_c > fan64_target[fan] + 2) {
        fan64_pwm_v[fan] += FAN64_STEP;
        if (fan64_pwm_v[fan] > 100) fan64_pwm_v[fan] = 100;
    } else if (temp_c < fan64_target[fan] - 2) {
        fan64_pwm_v[fan] -= FAN64_STEP;
        if (fan64_pwm_v[fan] < 0) fan64_pwm_v[fan] = 0;
    }
    return fan64_pwm_v[fan];
}

int fan64_pwm(int fan) {
    fan64_ensure();
    if (fan < 0 || fan >= FAN64_MAX) return -1;
    return fan64_pwm_v[fan];
}

/* Dogrusal egri: 30C->20%%, 80C->100%% arasi. */
int fan64_curve(int temp_c) {
    if (temp_c <= 30) return 20;
    if (temp_c >= 80) return 100;
    return 20 + (temp_c - 30) * 80 / 50;
}
