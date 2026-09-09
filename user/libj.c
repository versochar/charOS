/* 16E: DT_NEEDED kaynağı — libk.so'ya bağımlı. */
extern int k_val(void);
int j_run(void) { return k_val() + 1; }