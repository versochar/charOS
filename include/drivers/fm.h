#ifndef CHAROS_DRIVERS_FM_H
#define CHAROS_DRIVERS_FM_H

/* 18K: GUI Dosya Yöneticisi — liste, sürükle-bırak (move), chFS. */

int fm_init(void);      /* pencere + liste. 0 hazır. */
void fm_set_path(const char* path);
void fm_refresh(void);  /* güncel dizini yeniden listele */
int  fm_selftest(void); /* liste + sürükle-bırak move doğrula. 0 PASS. */

int fm_window_id(void);
/* 18P: bırakma hedefi için o anki dizin (çıktı NUL-sonlu) */
void fm_get_path(char* out, int max);

#endif
