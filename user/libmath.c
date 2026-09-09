/* 15A: örnek paylaşımlı kütüphane (ET_DYN). Bağımsız, dış simgesiz. */
int lib_add(int a, int b) { return a + b; }
int lib_mul(int a, int b) { return a * b; }
int lib_fact(int n) {
    int r = 1;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}
