/* 16D: lazy PLT testi — la_hook ilk çağrıda çözülür. */
extern int la_hook(int);
int la_call(int x) { return la_hook(x); }