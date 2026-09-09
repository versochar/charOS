/* 34B: boot services cleanup — acik handle kaydi.
 * ExitBootServices oncesi tum handle'lar kapatilmali; kayit saf mantiktir
 * (host testi uygun). Kapatma cagrilari loader.c'de BS ile olur.
 */
#ifndef CHAROS_CLEANUP64_H
#define CHAROS_CLEANUP64_H

#define CLEANUP64_MAX 16

int cleanup64_track(void *handle); /* 0=ok, -1=dolu, -2=cift kayit */
int cleanup64_count(void);
void *cleanup64_get(int i);
void cleanup64_untrack(void *handle);
void cleanup64_clear(void);

#endif
