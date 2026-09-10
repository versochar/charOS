#include "arch/x86_64/parttool.h"
#include <string.h>

#define PARTTOOL_DEV_USABLE 34   /* koruyucu bolgeler dahil, secili partitionlari ayirt */
#define PARTTOOL_MIN_LBA    2048 /* GPT kopyasi ve metaboltgen icin bayrak */

struct pt_part {
    char name[PARTTOOL64_NAME_MAX];
    int  type_code;
    u64  first_lba;
    u64  last_lba;
    u8   used;
};
struct pt_dev {
    u64  dev_id;
    u64  total_lba;
    u64  crc;
    int  corrupted;
    struct pt_part parts[PARTTOOL64_MAX_PARTS];
    int  nparts;
    u8   active;
};
static struct pt_dev gpt_devs[PARTTOOL64_MAX_DEVS];
static int parttool_inited = 0;

int parttool64_init(void) {
    parttool_inited = 0;
    memset(gpt_devs, 0, sizeof(gpt_devs));
    parttool_inited = 1;
    return 0;
}

static struct pt_dev *find_dev(u64 dev_id) {
    int i;
    if (!parttool_inited) return 0;
    for (i = 0; i < PARTTOOL64_MAX_DEVS; i++) {
        if (gpt_devs[i].active && gpt_devs[i].dev_id == dev_id)
            return &gpt_devs[i];
    }
    return 0;
}

static u64 crc_fnv(const struct pt_dev *d) {
    u64 h = 1469598103934665603ULL ^ d->dev_id;
    int i;
    for (i = 0; i < d->nparts; i++) {
        int j;
        for (j = 0; j < PARTTOOL64_NAME_MAX && d->parts[i].name[j]; j++) {
            h ^= (unsigned char)d->parts[i].name[j];
            h *= 1099511628211ULL;
        }
        h ^= (u64)d->parts[i].type_code;
        h ^= d->parts[i].first_lba;
        h ^= d->parts[i].last_lba;
        h *= 1099511628211ULL;
    }
    h ^= d->total_lba;
    h *= 1099511628211ULL;
    return h ? h : 1;
}

int parttool64_gpt_init(u64 dev_id, u64 total_lba) {
    int i;
    struct pt_dev *d;
    if (total_lba < PARTTOOL_DEV_USABLE || total_lba > (1ULL << 40))
        return -1;
    d = find_dev(dev_id);
    if (d) return -2; /* zaten formatli */
    for (i = 0; i < PARTTOOL64_MAX_DEVS; i++) {
        if (!gpt_devs[i].active) {
            gpt_devs[i].dev_id = dev_id;
            gpt_devs[i].total_lba = total_lba;
            gpt_devs[i].nparts = 0;
            gpt_devs[i].corrupted = 0;
            gpt_devs[i].active = 1;
            gpt_devs[i].crc = crc_fnv(&gpt_devs[i]);
            parttool_inited = 1;
            return 0;
        }
    }
    return -3;
}

int parttool64_gpt_add(u64 dev_id, const char *name, int type_code,
                       u64 first_lba, u64 last_lba) {
    struct pt_dev *d = find_dev(dev_id);
    struct pt_part *p;
    int i;
    if (!d) return -1;
    if (!name || !type_code) return -2;
    if (last_lba <= first_lba || last_lba >= d->total_lba)
        return -3; /* gecersiz aralik */
    if (first_lba < 2048 && type_code != PARTTOOL_TYPE_EFI)
        return -4; /* koruyucu bolgeye dokunma */
    if (d->nparts >= PARTTOOL64_MAX_PARTS) return -5;
    if (d->corrupted) return -6; /* bozuk tablo yazilamaz */
    /* cakis denetimi */
    for (i = 0; i < d->nparts; i++) {
        struct pt_part *q = &d->parts[i];
        if (first_lba < q->last_lba && q->first_lba < last_lba)
            return -7;
    }
    p = &d->parts[d->nparts];
    memset(p, 0, sizeof(*p));
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->type_code = type_code;
    p->first_lba = first_lba;
    p->last_lba = last_lba;
    p->used = 1;
    d->nparts++;
    d->crc = crc_fnv(d);
    return 0;
}

int parttool64_gpt_info(u64 dev_id, int index, char *name_out, int name_max,
                        int *type_out, u64 *first_out, u64 *last_out) {
    struct pt_dev *d = find_dev(dev_id);
    struct pt_part *p;
    if (!d) return -1;
    if (index < 0 || index >= d->nparts) return -2;
    p = &d->parts[index];
    if (name_out && name_max > 0) {
        strncpy(name_out, p->name, (size_t)name_max - 1);
        name_out[name_max - 1] = '\0';
    }
    if (type_out) *type_out = p->type_code;
    if (first_out) *first_out = p->first_lba;
    if (last_out) *last_out = p->last_lba;
    return 0;
}

int parttool64_gpt_delete(u64 dev_id, int index) {
    struct pt_dev *d = find_dev(dev_id);
    int i;
    if (!d) return -1;
    if (index < 0 || index >= d->nparts) return -2;
    if (d->corrupted) return -3;
    for (i = index; i < d->nparts - 1; i++)
        d->parts[i] = d->parts[i + 1];
    memset(&d->parts[d->nparts - 1], 0, sizeof(d->parts[0]));
    d->nparts--;
    d->crc = crc_fnv(d);
    return 0;
}

int parttool64_gpt_crc(u64 dev_id) {
    struct pt_dev *d = find_dev(dev_id);
    if (!d) return 0;
    return (int)(d->crc & 0x7FFFFFFF);
}

int parttool64_gpt_repair(u64 dev_id) {
    struct pt_dev *d = find_dev(dev_id);
    if (!d) return -1;
    d->corrupted = 0;
    d->crc = crc_fnv(d);
    return 0;
}

int parttool64_gpt_count(u64 dev_id) {
    struct pt_dev *d = find_dev(dev_id);
    if (!d) return -1;
    return d->nparts;
}