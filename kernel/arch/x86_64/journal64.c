/* 40B: journal (write-ahead) — sahnele/commit/abort + kurtarma tekrari. */
#include "arch/x86_64/longmode.h"

#define JOURNAL64_MAX 32
#define JOURNAL64_BSIZE 1024

struct journal64_entry {
    int used;
    u32 block;
    unsigned char data[JOURNAL64_BSIZE];
};

static struct journal64_entry journal64_stage[JOURNAL64_MAX];
static struct journal64_entry journal64_committed[JOURNAL64_MAX];
static int journal64_nstage = 0;
static int journal64_ncommitted = 0;
static int journal64_active = 0;
static u64 journal64_sequence = 0;

int journal64_begin(void) {
    int i;
    if (journal64_active) return -1; /* ic ice islem yok */
    for (i = 0; i < JOURNAL64_MAX; i++) journal64_stage[i].used = 0;
    journal64_nstage = 0;
    journal64_active = 1;
    return 0;
}

int journal64_write(u32 block, const void *data) {
    const unsigned char *s;
    int i;
    if (!journal64_active || !data) return -1;
    for (i = 0; i < JOURNAL64_MAX; i++) {
        if (journal64_stage[i].used && journal64_stage[i].block == block) {
            s = (const unsigned char *)data;
            for (int j = 0; j < JOURNAL64_BSIZE; j++)
                journal64_stage[i].data[j] = s[j];
            return 0;
        }
    }
    for (i = 0; i < JOURNAL64_MAX; i++) {
        if (!journal64_stage[i].used) {
            journal64_stage[i].used = 1;
            journal64_stage[i].block = block;
            s = (const unsigned char *)data;
            for (int j = 0; j < JOURNAL64_BSIZE; j++)
                journal64_stage[i].data[j] = s[j];
            journal64_nstage++;
            return 0;
        }
    }
    return -2; /* sahne dolu */
}

/* Backend baglantisi: ext2 yazici disaridan kurulur (40J testte). */
static int (*journal64_backend_wr)(u32 blk, const void *buf) = 0;

void journal64_set_backend(int (*wr)(u32 blk, const void *buf)) {
    journal64_backend_wr = wr;
}

int journal64_commit(void) {
    int i;
    if (!journal64_active) return -1;
    for (i = 0; i < JOURNAL64_MAX; i++) {
        if (!journal64_stage[i].used) continue;
        if (journal64_backend_wr)
            journal64_backend_wr(journal64_stage[i].block,
                                 journal64_stage[i].data);
        /* Son commit kopyasini sakla (recover icin) */
        if (journal64_ncommitted < JOURNAL64_MAX) {
            int k;
            for (k = 0; k < JOURNAL64_MAX; k++) {
                if (!journal64_committed[k].used) {
                    journal64_committed[k].used = 1;
                    journal64_committed[k].block = journal64_stage[i].block;
                    for (int j = 0; j < JOURNAL64_BSIZE; j++)
                        journal64_committed[k].data[j] =
                            journal64_stage[i].data[j];
                    journal64_ncommitted++;
                    break;
                }
            }
        }
        journal64_stage[i].used = 0;
    }
    journal64_nstage = 0;
    journal64_active = 0;
    journal64_sequence++;
    return (int)journal64_sequence;
}

int journal64_abort(void) {
    int i;
    if (!journal64_active) return -1;
    for (i = 0; i < JOURNAL64_MAX; i++) journal64_stage[i].used = 0;
    journal64_nstage = 0;
    journal64_active = 0;
    return 0;
}

int journal64_recover(void) {
    int i, n = 0;
    if (!journal64_backend_wr) return -1;
    for (i = 0; i < JOURNAL64_MAX; i++) {
        if (!journal64_committed[i].used) continue;
        journal64_backend_wr(journal64_committed[i].block,
                             journal64_committed[i].data);
        n++;
    }
    return n;
}

u64 journal64_seq(void) { return journal64_sequence; }
