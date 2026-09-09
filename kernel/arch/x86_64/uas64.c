/* 45A: UAS mass storage — komut IU kurma + sense + etiket yonetimi. */
#include "arch/x86_64/longmode.h"

#define UAS64_MAX_TAGS 64
#define UAS64_IU_COMMAND 0x01
#define UAS64_IU_SENSE 0x03

static u64 uas64_tagmap = 0;
static int uas64_aborted[UAS64_MAX_TAGS];

int uas64_build_cmd(u8 lun, const u8 *cdb, int cdb_len, u16 tag,
                    struct uas64_cmd *out) {
    int i;
    if (!cdb || cdb_len <= 0 || cdb_len > 16 || !out) return -1;
    if (lun > 7) return -2;
    out->iu_id = UAS64_IU_COMMAND;
    out->rsvd = 0;
    out->tag = tag;
    out->prio = 0;
    out->lun = lun;
    out->cdb_len = (u8)cdb_len;
    for (i = 0; i < 16; i++)
        out->cdb[i] = i < cdb_len ? cdb[i] : 0;
    return 0;
}

/* Sense IU: [0]=iu_id, [2..3]=tag, [4]=status, [12]=key, [13]=ASC. */
int uas64_parse_sense(const unsigned char *iu, int len, u8 *key, u8 *asc) {
    if (!iu || len < 14) return -1;
    if (iu[0] != UAS64_IU_SENSE) return -2;
    if (key) *key = iu[12] & 0x0F;
    if (asc) *asc = iu[13];
    return iu[4]; /* durum bayti */
}

int uas64_tag_alloc(void) {
    int i;
    for (i = 0; i < UAS64_MAX_TAGS; i++) {
        if (!(uas64_tagmap & (1ULL << i))) {
            uas64_tagmap |= (1ULL << i);
            uas64_aborted[i] = 0;
            return i;
        }
    }
    return -1;
}

void uas64_tag_free(int tag) {
    if (tag < 0 || tag >= UAS64_MAX_TAGS) return;
    uas64_tagmap &= ~(1ULL << tag);
    uas64_aborted[tag] = 0;
}

int uas64_abort(u16 tag) {
    if (tag >= UAS64_MAX_TAGS) return -1;
    if (!(uas64_tagmap & (1ULL << tag))) return -2;
    uas64_aborted[tag] = 1;
    return 0;
}
