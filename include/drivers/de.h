#ifndef CHAROS_DRIVERS_DE_H
#define CHAROS_DRIVERS_DE_H

/* 18T: Performans & bütünleştirme — vsync zamanlamalı sunum + DE'nin
 * açılışta otomatik başladığının doğrulanması (hazırlık denetimi +
 * açılış ekranı + masaüstü geri yükleme). */

int de_boot(void);       /* tüm DE bileşenleri hazır mı? Dönüş: hazır sayısı */
int de_selftest(void);   /* vsync/sayaç + boot/splash/restore. 0 PASS. */

#endif
