/* 40E: POSIX ACL — etiket listesi + maske mantigi.
 * Degerlendirme: sahip -> USER_OBJ; adli kullanici; grup (sahip grubu +
 * adli gruplar, MASK'e tabi); diger -> OTHER.
 */
#include "arch/x86_64/longmode.h"

#define ACL64_MAX 64

struct acl64_entry {
    int used;
    u64 ino;
    int tag;
    u32 id;
    int perm; /* r=4,w=2,x=1 */
};

static struct acl64_entry acl64_tab[ACL64_MAX];

int acl64_set(u64 ino, int tag, u32 id, int perm) {
    int i;
    if (!ino || tag < ACL64_USER_OBJ || tag > ACL64_OTHER) return -1;
    if (perm < 0 || perm > 7) return -1;
    for (i = 0; i < ACL64_MAX; i++) {
        if (acl64_tab[i].used && acl64_tab[i].ino == ino &&
            acl64_tab[i].tag == tag && acl64_tab[i].id == id) {
            acl64_tab[i].perm = perm;
            return 0;
        }
    }
    for (i = 0; i < ACL64_MAX; i++) {
        if (!acl64_tab[i].used) {
            acl64_tab[i].used = 1;
            acl64_tab[i].ino = ino;
            acl64_tab[i].tag = tag;
            acl64_tab[i].id = id;
            acl64_tab[i].perm = perm;
            return 0;
        }
    }
    return -2;
}

static int acl64_perm(u64 ino, int tag, u32 id, int *found) {
    int i;
    if (found) *found = 0;
    for (i = 0; i < ACL64_MAX; i++) {
        if (!acl64_tab[i].used || acl64_tab[i].ino != ino) continue;
        if (acl64_tab[i].tag != tag) continue;
        if ((tag == ACL64_USER || tag == ACL64_GROUP) &&
            acl64_tab[i].id != id)
            continue;
        if (found) *found = 1;
        return acl64_tab[i].perm;
    }
    return 0;
}

/* 1=serbest, 0=engelli */
int acl64_check(u64 ino, u32 uid, u32 gid, int req) {
    int f = 0, p;
    if (!ino || req < 0 || req > 7) return 0;
    /* Sahip: USER_OBJ (maskesiz) */
    if (uid == 0) {
        /* KOK: USER_OBJ girdisi varsa ona bak, yoksa serbest (skeleton) */
        p = acl64_perm(ino, ACL64_USER_OBJ, 0, &f);
        if (f) return ((p & req) == req) ? 1 : 0;
        return 1;
    }
    /* Adli kullanici */
    p = acl64_perm(ino, ACL64_USER, uid, &f);
    if (f) {
        int m = acl64_perm(ino, ACL64_MASK, 0, &f);
        if (f) p &= m;
        return ((p & req) == req) ? 1 : 0;
    }
    /* Grup sinifi: GROUP_OBJ + adli gruplar (MASK'e tabi) */
    {
        int gperm = 0, has = 0;
        int m, mf = 0;
        p = acl64_perm(ino, ACL64_GROUP_OBJ, 0, &f);
        if (f) {
            gperm |= p;
            has = 1;
        }
        p = acl64_perm(ino, ACL64_GROUP, gid, &f);
        if (f) {
            gperm |= p;
            has = 1;
        }
        if (has) {
            m = acl64_perm(ino, ACL64_MASK, 0, &mf);
            if (mf) gperm &= m;
            return ((gperm & req) == req) ? 1 : 0;
        }
    }
    /* Diger */
    p = acl64_perm(ino, ACL64_OTHER, 0, &f);
    if (f) return ((p & req) == req) ? 1 : 0;
    return 0;
}
