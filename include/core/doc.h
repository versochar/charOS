#ifndef CHAROS_CORE_DOC_H
#define CHAROS_CORE_DOC_H

/* 29.2: Makine-okunur syscall belgeleri.
 * Kullanıcı alanı `man` benzeri sorgu yapar (SYS_DOCNAME/SYS_DOCDESC).
 * Aynı dosya çekirdekte ve host testinde derlenir (yalnızca stdint).
 */
#include "stdint.h"

uint32_t    doc_count(void);
const char* doc_name(uint32_t nr); /* NULL: tanımsız */
const char* doc_desc(uint32_t nr); /* NULL: tanımsız */
int         doc_copy(const char* s, char* out, uint32_t max); /* len / -1 */
int         doc_selftest(void);    /* 0 ok; sıralılık + boşluk denetimi */

#endif
