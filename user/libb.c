/* 15D: tanımsız fonksiyon + veri sembolleri — çözüm sırasıyla interpose olur. */
extern int shared_hook(int);
extern int liba_g;

int b_run(int x) { return shared_hook(x) + 1; }
int b_var(void) { return liba_g + 2; }