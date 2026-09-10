#ifndef CHAROS_CORE_VERSION_H
#define CHAROS_CORE_VERSION_H

/* 27.2: Derleme-zamanı sürüm bilgisi.
 * Değerler Makefile'dan -D ile gelir (VERSION dosyası + git commit);
 * tanımsızsa geliştirme varsayılanı kullanılır. Salt-okunur veri.
 */
const char* version_string(void);
const char* version_commit(void);

#endif
