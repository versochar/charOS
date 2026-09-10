#include "arch/x86_64/recenv.h"

#define RECENV64_MAX_FAILS 3

static int rec_state = RECENV_NONE;
static int rec_fail = 0;
static int rec_selected = RECENV_MODE_SAFE;
static int rec_timeout_sec = 10;
static int rec_count = -1; /* -1 = girilmemis */

int recenv64_init(void) {
    rec_state = RECENV_NONE;
    rec_fail = 0;
    rec_selected = RECENV_MODE_SAFE;
    rec_timeout_sec = 10;
    rec_count = -1;
    return 0;
}

int recenv64_boot_failed(void) {
    if (rec_fail >= RECENV64_MAX_FAILS) return 0; /* zaten max */
    rec_fail++;
    return rec_fail;
}

int recenv64_menu_timeout(int seconds) {
    if (seconds < 0 || seconds > 300) return -1;
    rec_timeout_sec = seconds;
    return 0;
}

int recenv64_enter(void) {
    if (rec_state == RECENV_RESOLVED) return -1; /* zaten karar verildi */
    rec_state = RECENV_MENU;
    rec_count = rec_timeout_sec;
    return 0;
}

int recenv64_select(int mode) {
    if (rec_state != RECENV_MENU) return -2;
    if (mode < 0 || mode >= RECENV_MODES_N) return -3;
    rec_selected = mode;
    return 0;
}

int recenv64_confirm(void) {
    if (rec_state != RECENV_MENU) return -1;
    rec_state = RECENV_RESOLVED;
    rec_count = 0;
    return 0;
}

int recenv64_countdown(int *seconds_left) {
    if (!seconds_left) return -1;
    if (rec_state == RECENV_NONE) return -1;
    if (rec_state == RECENV_RESOLVED) {
        *seconds_left = 0;
        return 0;
    }
    if (rec_count > 0) rec_count--;
    *seconds_left = rec_count;
    if (rec_count == 0) {
        /* zaman doldu: safe mode oto secim + karar verildi */
        rec_selected = rec_selected >= RECENV_MODES_N ? RECENV_MODE_SAFE
                                                      : rec_selected;
        rec_state = RECENV_RESOLVED;
        return 1;
    }
    return 0;
}

int recenv64_state(int *out) {
    if (!out) return -1;
    *out = rec_state;
    return 0;
}

int recenv64_selected_mode(int *out) {
    if (!out) return -1;
    *out = rec_selected;
    return 0;
}