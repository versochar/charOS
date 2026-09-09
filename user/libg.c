/* 15G: constructor/destructor (init/fini dizileri) + export çağrısı. */
extern void prog_oninit(void);
extern void prog_onfini(void);

static int g_state;

__attribute__((constructor)) static void g_ctor(void) {
    g_state = 1;
    prog_oninit();
}
__attribute__((destructor)) static void g_fini(void) {
    g_state = 2;
    prog_onfini();
}

int g_get(void) { return g_state; }