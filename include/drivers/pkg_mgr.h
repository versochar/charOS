#ifndef CHAROS_DRIVERS_PKG_MGR_H
#define CHAROS_DRIVERS_PKG_MGR_H

#include <stdint.h>

/* 29E: Paket yöneticisi - kur-kullan döngüsü (skeleton).
 * Gerçek paket sistemi (kurulum, bağımlılık, güncelleme) ileri aşama. */

/* Paket listesi (statik) */
int pkg_init(void);
int pkg_install(const char* name);
int pkg_list(void);  /* kurulu paketleri listele */
void pkg_stats(void);
int pkg_selftest(void);

#endif
