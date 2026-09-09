/* 15D/15G: program export'una (dl_register_export) bağımlı kütüphane. */
extern int prog_version(void);

int e_run(void) { return prog_version() + 1; }