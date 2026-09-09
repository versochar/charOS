#ifndef CHAROS_DRIVERS_GPT_H
#define CHAROS_DRIVERS_GPT_H

#include <stdint.h>

/* 27C: GPT bölüm tablosu tarama (NVMe/virtio üstünde çalışır).
 * Okuma arayüzü enjekte edilir: read512(lba, buf512) 0=ok. */

#define GPT_MAX_PARTS 16

struct gpt_part {
    uint8_t type_guid[16];
    uint8_t uniq_guid[16];
    uint64_t start_lba;
    uint64_t end_lba;   /* dahil */
    char name[37];      /* UTF-16 -> ASCII, NUL sonlu */
};

/* charOS veri bölümü GUID'i (sfdisk damgasıyla aynı olmalı):
 * A1B2C3D4-E5F6-4A7B-8C9D-E0F1A2B3C4D5 (karışık endian kaydı!) */
extern const uint8_t CHAROS_PART_GUID[16];

/* Diski tara: header CRC + girişleri doğrular.
 * Dönüş: bölüm sayısı (>=0), -1 GPT yok/bozuk. parts[] doldurulur. */
int gpt_scan(int (*read512)(uint32_t lba, void* buf),
             struct gpt_part* parts, int maxparts);

/* parts[] içinde charOS bölümü ara (0-tabanlı indeks) ya da -1 */
int gpt_find_charos(const struct gpt_part* parts, int n);

/* İlk boş (tip GUID sıfır) ve >= min_sectors bölümü ara ya da -1
 * (NOT: boş girişler taramada atlanır; ham modda çalışır, şimdilik -1) */
int gpt_find_empty(const struct gpt_part* parts, int n, uint64_t min_sectors);

/* NVMe taraması + önbellek (27D ile paylaşılır) */
int gpt_scan_nvme(void);              /* bölüm sayısı / -1 */
void gpt_invalidate(void);            /* yazma sonrası önbelleği düşür */
const struct gpt_part* gpt_cached(int* n); /* 0 = taranmadı/yok */

/* Boot self-test (NVMe üstünde): 0 PASS, -1 FAIL/atlandı */
int gpt_selftest(void);

#endif
