/* 35C: swap LRU reclaim — aktif/pasif kuyruk + swap'a bosaltma.
 * Evict: pasif bastan alir, swap slotuna yazar; aktif bitince pasife iner.
 */
#include "arch/x86_64/longmode.h"

#define RECLAIM64_MAX 256

static u64 reclaim64_active[RECLAIM64_MAX];
static u64 reclaim64_inactive[RECLAIM64_MAX];
static int reclaim64_na = 0;
static int reclaim64_ni = 0;
static int reclaim64_ready = 0;

int reclaim64_init(void) {
    reclaim64_na = 0;
    reclaim64_ni = 0;
    reclaim64_ready = 1;
    return 0;
}

static int find_active(u64 frame) {
    int i;
    for (i = 0; i < reclaim64_na; i++)
        if (reclaim64_active[i] == frame) return i;
    return -1;
}

static int find_inactive(u64 frame) {
    int i;
    for (i = 0; i < reclaim64_ni; i++)
        if (reclaim64_inactive[i] == frame) return i;
    return -1;
}

int reclaim64_add(u64 frame) {
    if (!reclaim64_ready || !frame) return -1;
    if (find_active(frame) >= 0 || find_inactive(frame) >= 0) return -2;
    if (reclaim64_na >= RECLAIM64_MAX) return -3;
    reclaim64_active[reclaim64_na++] = frame;
    return 0;
}

void reclaim64_touch(u64 frame) {
    int i, j;
    /* Pasifteyse aktife tasi (sifre: sona ekle) */
    i = find_inactive(frame);
    if (i < 0) return;
    for (j = i; j + 1 < reclaim64_ni; j++)
        reclaim64_inactive[j] = reclaim64_inactive[j + 1];
    reclaim64_ni--;
    if (reclaim64_na < RECLAIM64_MAX)
        reclaim64_active[reclaim64_na++] = frame;
}

int reclaim64_evict(int n, u64 *out) {
    int done = 0, i;
    if (!reclaim64_ready || n <= 0) return 0;
    while (done < n) {
        u64 frame;
        u64 slot;
        void *buf;
        if (!reclaim64_ni) {
            /* Pasif bos: aktifin yarisini indir */
            int move = reclaim64_na / 2;
            if (!move) break;
            for (i = 0; i < move && reclaim64_ni < RECLAIM64_MAX; i++) {
                reclaim64_inactive[reclaim64_ni++] = reclaim64_active[i];
            }
            for (i = 0; i + move < reclaim64_na; i++)
                reclaim64_active[i] = reclaim64_active[i + move];
            reclaim64_na -= move;
            continue;
        }
        frame = reclaim64_inactive[0];
        for (i = 0; i + 1 < reclaim64_ni; i++)
            reclaim64_inactive[i] = reclaim64_inactive[i + 1];
        reclaim64_ni--;
        if (swap64_alloc_slot(&slot) != 0) {
            /* Slot yok: frame'i basa iade et, dur */
            for (i = reclaim64_ni; i > 0; i--)
                reclaim64_inactive[i] = reclaim64_inactive[i - 1];
            if (reclaim64_ni < RECLAIM64_MAX)
                reclaim64_inactive[reclaim64_ni++] = frame;
            break;
        }
        buf = pmm64_frame_ptr(frame);
        if (swap64_out(slot, buf) != 0) {
            swap64_free_slot(slot);
            break;
        }
        pmm64_free_frame(frame);
        if (out) out[done] = slot;
        done++;
    }
    return done;
}
