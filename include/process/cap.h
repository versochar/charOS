#ifndef CHAROS_PROCESS_CAP_H
#define CHAROS_PROCESS_CAP_H

/* 22.2: Kanonik capability bitleri.
 * UYARI: bit1/2/3/5/6/7 include/core/syscall.h ile AYNI olmalı
 * (chfs.c ham bitlerle denetliyor, userprog.c bit1/bit5 sabitliyor).
 * 22.7-fix: ilk taslakta FOWNER/SETUID/SETGID bitleri çakışıyordu,
 * mevcut ABI (syscall.h) kazanacak şekilde düzeltildi.
 */
#include "stdint.h"

#define CAP_CHOWN         (1u << 0)  /* 22.2 yeni */
#define CAP_DAC_OVERRIDE  (1u << 1)  /* mevcut: syscall.h + userprog */
#define CAP_DAC_READ_SEARCH (1u << 2) /* mevcut: syscall.h */
#define CAP_FOWNER        (1u << 3)  /* mevcut: syscall.h + chfs.c */
#define CAP_FSETID        (1u << 4)  /* 22.2 yeni */
#define CAP_KILL          (1u << 5)  /* mevcut: syscall.h + userprog */
#define CAP_SETGID        (1u << 6)  /* mevcut: syscall.h */
#define CAP_SETUID        (1u << 7)  /* mevcut: syscall.h */
#define CAP_SYS_RAWIO     (1u << 8)  /* 22.2 yeni (ham disk) */
#define CAP_SYS_ADMIN     (1u << 9)  /* 22.2 yeni */
#define CAP_NET_ADMIN     (1u << 10) /* 22.2 yeni */
#define CAP_SYSLOG        (1u << 11) /* 22.2 yeni */
#define CAP_SYS_MODULE    (1u << 12) /* 22.2 yeni */
#define CAP_ALL           0x1FFFu
#define CAP_VALID_MASK    CAP_ALL

/* Denetim kaydı (22.3: audit halkası) */
struct cap_audit_entry {
    uint32_t cap;
    uint32_t granted; /* 1=verildi, 0=reddedildi */
};

/* Saf bitmask mantığı: task yapısına dokunmaz, host'ta da derlenir. */
int      cap_valid(uint32_t cap);                 /* tek bit ve maske içinde mi */
int      cap_has(uint32_t set, uint32_t cap);     /* 1=var, 0=yok */
int      cap_grant(uint32_t* set, uint32_t cap);  /* bit ekle: 0 ok / -1 hata */
int      cap_revoke(uint32_t* set, uint32_t cap); /* bit sil: 0 ok / -1 hata */
int      cap_drop_all(uint32_t* set);             /* hepsini temizle */
int      cap_is_subset(uint32_t a, uint32_t b);   /* a ⊆ b mi */
uint32_t cap_allow_only(uint32_t cur, uint32_t want); /* want ⊆ cur ? want : cur */

/* Denetim halkası (son 16 karar) */
void cap_audit(uint32_t cap, uint32_t granted);
int  cap_audit_read(uint32_t idx, uint32_t* cap, uint32_t* granted);

#endif
