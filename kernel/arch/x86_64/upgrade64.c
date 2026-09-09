/* 41F: atomic upgrade — A/B slot + hazirla/cevir/geri-al. */
#include "arch/x86_64/longmode.h"

static char upgrade64_ver[2][16];
static int upgrade64_has[2] = {0, 0};
static int upgrade64_active_slot = 0;
static char upgrade64_staged[16];
static int upgrade64_staged_ok = 0;

static void upg_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

int upgrade64_stage(const char *ver) {
    if (!ver || !ver[0]) return -1;
    upg_copy(upgrade64_staged, ver, 16);
    upgrade64_staged_ok = 1;
    return 0;
}

int upgrade64_commit(void) {
    int next;
    if (!upgrade64_staged_ok) return -1;
    next = upgrade64_active_slot ^ 1;
    upg_copy(upgrade64_ver[next], upgrade64_staged, 16);
    upgrade64_has[next] = 1;
    upgrade64_active_slot = next;
    upgrade64_staged_ok = 0;
    return 0;
}

int upgrade64_rollback(void) {
    int prev = upgrade64_active_slot ^ 1;
    if (!upgrade64_has[prev]) return -1; /* donulecek slot yok */
    upgrade64_active_slot = prev;
    upgrade64_staged_ok = 0;
    return 0;
}

int upgrade64_active(void) { return upgrade64_active_slot; }

int upgrade64_version(int slot, char *out, int max) {
    if (slot < 0 || slot > 1 || !upgrade64_has[slot]) return -1;
    if (out && max > 0) upg_copy(out, upgrade64_ver[slot], max);
    return 0;
}
