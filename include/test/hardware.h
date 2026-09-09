#ifndef CHAROS_TEST_HARDWARE_H
#define CHAROS_TEST_HARDWARE_H

/* 30B: Donanım test paketi - bütünleşik PASS doğrulama.
 * Tüm serilerin (26-29) sonuçlarının tek noktada doğrulanması. */

/* Tüm entegre test sonuçlarını topla */
int hardware_integration_test(void);

/* PASS sayısını yazdır */
void integration_report(void);

#endif
