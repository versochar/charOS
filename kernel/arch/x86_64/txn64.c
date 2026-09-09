/* 41E: transaction — kur/kaldir op gunlugu + commit/rollback.
 * Kurulum veritabani: ad -> surum (bellek-ici, kalici depo 42x'te).
 */
#include "arch/x86_64/longmode.h"

#define TXN64_MAX_OPS 64
#define TXN64_MAX_DB 128

struct txn64_db_entry {
    int used;
    char name[32];
    char ver[16];
};

static struct txn64_db_entry txn64_db[TXN64_MAX_DB];

struct txn64_op {
    int kind; /* 0=kur, 1=kaldir */
    char name[32];
    char ver[16];
    char oldver[16];
    int had_old;
};

static struct txn64_op txn64_ops[TXN64_MAX_OPS];
static int txn64_nops = 0;
static int txn64_active = 0;

static void txn_str_copy(char *d, const char *s, int n) {
    int i;
    for (i = 0; i + 1 < n && s[i]; i++) d[i] = s[i];
    d[i] = 0;
}

static int txn_str_eq(const char *a, const char *b) {
    int i;
    for (i = 0;; i++) {
        if (a[i] != b[i]) return 0;
        if (!a[i]) return 1;
    }
}

static struct txn64_db_entry *txn64_find(const char *name) {
    int i;
    for (i = 0; i < TXN64_MAX_DB; i++)
        if (txn64_db[i].used && txn_str_eq(txn64_db[i].name, name))
            return &txn64_db[i];
    return 0;
}

int txn64_begin(void) {
    if (txn64_active) return -1;
    txn64_nops = 0;
    txn64_active = 1;
    return 0;
}

int txn64_install(const char *name, const char *ver) {
    struct txn64_op *o;
    struct txn64_db_entry *e;
    if (!txn64_active || !name || !ver) return -1;
    if (txn64_nops >= TXN64_MAX_OPS) return -2;
    o = &txn64_ops[txn64_nops++];
    o->kind = 0;
    txn_str_copy(o->name, name, 32);
    txn_str_copy(o->ver, ver, 16);
    e = txn64_find(name);
    o->had_old = e ? 1 : 0;
    txn_str_copy(o->oldver, e ? e->ver : "", 16);
    return 0;
}

int txn64_remove(const char *name) {
    struct txn64_op *o;
    struct txn64_db_entry *e;
    if (!txn64_active || !name) return -1;
    e = txn64_find(name);
    if (!e) return -2; /* kurulu degil */
    if (txn64_nops >= TXN64_MAX_OPS) return -3;
    o = &txn64_ops[txn64_nops++];
    o->kind = 1;
    txn_str_copy(o->name, name, 32);
    txn_str_copy(o->ver, e->ver, 16);
    o->had_old = 1;
    txn_str_copy(o->oldver, e->ver, 16);
    return 0;
}

static void txn64_apply_install(const char *name, const char *ver) {
    struct txn64_db_entry *e = txn64_find(name);
    int i;
    if (e) {
        txn_str_copy(e->ver, ver, 16);
        return;
    }
    for (i = 0; i < TXN64_MAX_DB; i++) {
        if (!txn64_db[i].used) {
            txn64_db[i].used = 1;
            txn_str_copy(txn64_db[i].name, name, 32);
            txn_str_copy(txn64_db[i].ver, ver, 16);
            return;
        }
    }
}

static void txn64_apply_remove(const char *name) {
    struct txn64_db_entry *e = txn64_find(name);
    if (e) e->used = 0;
}

int txn64_commit(void) {
    int i;
    if (!txn64_active) return -1;
    for (i = 0; i < txn64_nops; i++) {
        if (txn64_ops[i].kind == 0)
            txn64_apply_install(txn64_ops[i].name, txn64_ops[i].ver);
        else
            txn64_apply_remove(txn64_ops[i].name);
    }
    txn64_nops = 0;
    txn64_active = 0;
    return 0;
}

int txn64_rollback(void) {
    int i;
    if (!txn64_active) return -1;
    /* Tersine sira: kurulanlari kaldir, kaldirilanlari geri yukle */
    for (i = txn64_nops - 1; i >= 0; i--) {
        if (txn64_ops[i].kind == 0) {
            if (txn64_ops[i].had_old)
                txn64_apply_install(txn64_ops[i].name,
                                    txn64_ops[i].oldver);
            else
                txn64_apply_remove(txn64_ops[i].name);
        } else {
            txn64_apply_install(txn64_ops[i].name, txn64_ops[i].oldver);
        }
    }
    txn64_nops = 0;
    txn64_active = 0;
    return 0;
}

int txn64_installed(const char *name, char *ver_out, int max) {
    struct txn64_db_entry *e;
    if (!name) return -1;
    e = txn64_find(name);
    if (!e) return -2;
    if (ver_out && max > 0) txn_str_copy(ver_out, e->ver, max);
    return 0;
}
