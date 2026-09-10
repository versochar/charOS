#ifndef CHAROS_CORE_AUTH_H
#define CHAROS_CORE_AUTH_H

/* 37.2: Otomasyon tasarım (gerçek çekirdek kodu).
 * Tanım: çekirdekte çalışan, host'ta da derlenen auth katmanı.
 */
#include "stdint.h"

/* 37.3: Kimlik doğrulama durumları */
#define AUTH_OK 0
#define AUTH_FAIL -1

/* 37.3: Kimlik doğrulama başlatma */
void auth_init(void);

/* 37.3: Kimlik doğrulama çağrısı (örnek: kullanıcı adı + şifre simülasyonu) */
int auth_check(const char* name, uint32_t token);

/* 37.3: Token üretimi (örnek: sabit + zamana bağlı türetme) */
uint32_t auth_token_gen(const char* name, uint32_t seed);

#endif
