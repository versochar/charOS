/* 15D: interposition kaynağı — fonksiyon + veri sembolleri (preemptable). */
int shared_hook(int x) { return x * 3; }
int liba_g = 5;
int liba_set_g(int v) { liba_g = v; return v; }
int liba_get_g(void) { return liba_g; }